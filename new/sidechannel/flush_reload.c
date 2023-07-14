#include "flush_reload.h"


//#define maccess(p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

void __attribute__((__always_inline__)) make_channel(char* oracle_ptr, size_t offset){
    oracle_ptr = malloc(256 * offset);
    flush(oracle_ptr, offset);
}

void __attribute__((__always_inline__)) free_channel(char* oracle_ptr){
    free(oracle_ptr);
}

inline int __attribute__((__always_inline__)) flush_reload_t(void *ptr) {
    register uint64_t start = 0, end = 0;
    static unsigned int t_junk = 0;
    start = __rdtscp(&t_junk);
    maccess(ptr);
    //junk = *((int*) ptr);

    end = __rdtscp(&t_junk);
    _mm_mfence();
    _mm_clflush(ptr);

    return (int)(end - start);
}

void __attribute__((__always_inline__)) flush(char* oracle_ptr, size_t offset){
    static int i;
    for (i =0; i < 256; i++){
        _mm_clflush(oracle_ptr + i * offset);
    }
}

void __attribute__((__always_inline__)) probe(char* oracle_ptr, int thresh, size_t offset, int data[256]){
    // TODO: include second-best?
    static int i, mix_i, j;
    static char* addr;


    for (i = 0; i < 256; i++) {
        mix_i = ((i * 167) + 13) & 255;
        if (32 > mix_i || mix_i > 126){
            continue;
        }
        addr = oracle_ptr + mix_i * offset;
        if (flush_reload_t(addr) <= thresh){
            //printf("%c\n", mix_i);
            data[mix_i]++;
        }
        _mm_mfence();
    }
}
