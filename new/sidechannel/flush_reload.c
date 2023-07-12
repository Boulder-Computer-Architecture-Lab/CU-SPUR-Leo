#include "flush_reload.h"

void maccess(void *p) {asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

inline void make_channel(fr_channel *channel){
    channel->offset = 4096;
    channel->oracle_block = malloc(256 *channel->offset);
    for (int i =0; i < 256; i++){
        channel->results[i] = 0;
    }
    flush(channel);
}

inline void free_channel(fr_channel *channel){
    free(channel->oracle_block);
}

inline int flush_reload_t(void *ptr) {
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

inline void flush(fr_channel *channel){
    static int i;
    for (i =0; i < 256; i++){
        _mm_clflush(channel->oracle_block + i * channel->offset);
    }
}

inline int probe(fr_channel *channel, int thresh){
    // TODO: include second-best?
    int i, mix_i, j;
    char* addr;

    for (i = 0; i < 256; i++) {
        mix_i = ((i * 167) + 13) & 255;
        addr = channel->oracle_block + mix_i * channel->offset;
        if (flush_reload_t(addr) <= thresh && 32 <= mix_i && mix_i <= 126){
            channel->results[mix_i]++;
        }
    }

    j = -1;
    for (i = 0; i < 256; i++) {
      if (j < 0 || channel->results[i] > channel->results[j]) {
        j = i;
      }
    }
    return j;
}

inline void encode(fr_channel *channel, char data){
    *(channel->oracle_block + data * channel->offset) = 10;
}