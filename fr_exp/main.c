#include <emmintrin.h>
#include <x86intrin.h>

#include "../old/libcache/cache.h"

char *mem;
jmp_buf trycatch_buf;
size_t CACHE_MISS = 150;
size_t pagesize = 4096;
int junk;

int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  /* #if USE_RDTSC_BEGIN_END
    start = rdtsc_begin();
  #else
    start = rdtsc();
  #endif */
  maccess(ptr);


  end = __rdtscp(&junk);
  /* #if USE_RDTSC_BEGIN_END
    end = rdtsc_end();
  #else
    end = rdtsc();
  #endif */

  flush(ptr);

  return (int)(end - start);
}

void main(){
    //char tmp[256*pagesize];
    char *tmp = malloc(256*pagesize);
    FILE *fptr;
    int x;

    mfence();
    int mix_i;
    for (int i =0 ; i < 256; i++){
      flush(tmp + i * pagesize);
    }
    
    tmp[1*pagesize] = 10;
    fptr = fopen("fr_1.csv","w");
    for (int i =0; i < 256; i++){
        mix_i = ((i * 167) + 13) & 255;
        fprintf(fptr, "%d->%d:%d, ",i,mix_i,flush_reload_t(tmp+mix_i*pagesize));
    }
    fclose(fptr);


    mfence();

    for (int i =0 ; i < 256; i++){
      flush(tmp + i * pagesize);
    }
    

    mfence();
    fptr = fopen("fr_2.csv","w");
    for (int i =0; i < 256; i++){
        mix_i = ((i * 167) + 13) & 255;
        fprintf(fptr, "%d->%d:%d, ",i,mix_i,flush_reload_t(tmp+mix_i*pagesize));
    }
    fclose(fptr);
  
}