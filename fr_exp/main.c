#include <emmintrin.h>
#include <x86intrin.h>

#include "../old/libcache/cache.h"

char *mem;
jmp_buf trycatch_buf;
size_t CACHE_MISS = 150;
size_t pagesize = 4096;
int junk;

void __attribute__((__always_inline__)) maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }
int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);


  end = __rdtscp(&junk);

  _mm_mfence();
  _mm_clflush(ptr);

  return (int)(end - start);
}

void main(){
    //char tmp[256*pagesize];
    char *tmp = malloc(256*pagesize);
    int x;

    _mm_mfence();
    int mix_i;
    for (int i =0 ; i < 256; i++){
      _mm_clflush(tmp + i * pagesize);
    }
    
    _mm_mfence();
    tmp[1*pagesize] = 10;
    _mm_clflush(tmp);
    _mm_mfence();
    for (int i =0; i < 256; i++){
        mix_i = ((i * 167) + 13) & 255;
        if (flush_reload_t(tmp+mix_i*pagesize) <= 80){
          printf("hit at %d", mix_i);
        }
        _mm_mfence();
    }


    _mm_mfence();
    free(tmp);
  
}