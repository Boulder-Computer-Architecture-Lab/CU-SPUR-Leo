#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>

// experiment with a potentially more efficient method of accessing an entry within the flush-reload buffer

char* probe_buf;
char* loc_buf[256];
int junk = 0;

void __attribute__((__always_inline__)) maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }


int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  _mm_mfence();
  //flush(ptr);
  _mm_clflush(ptr);

  return (int)(end - start);
}

int main(){
    probe_buf = malloc(256*4096);

    for (int i = 0; i < 256; i++){
        _mm_clflush(probe_buf + i * 4096);
        loc_buf[i] = probe_buf + i*4096;
    }

    printf("location buffer: %p\n",loc_buf[10]);
    printf("actual location: %p\n",probe_buf + 10*4096);

    _mm_mfence();
    maccess(loc_buf[10]);

    _mm_mfence();
    int mix_i = 0;
    for (int i = 0; i < 256; i++) {
        mix_i = ((i * 167) + 13) & 255;
        if (flush_reload_t(probe_buf + mix_i * 4096) <= 80){
            printf("hit at %d\n",mix_i);
        }
    }

    // verify that locations in probe_buf aren't prefetched by accident

    printf("break \n\n");
    maccess(loc_buf[10]);

    for (int i = 0; i < 256; i++) {
        mix_i = ((i * 167) + 13) & 255;
        if (flush_reload_t(probe_buf + mix_i * 4096) <= 80){
            printf("hit at %d\n",mix_i);
        }
    }

    free(probe_buf);

}