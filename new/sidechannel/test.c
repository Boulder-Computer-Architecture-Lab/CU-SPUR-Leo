#include "flush_reload.h"
#include <stdio.h>

int main(){
    char* channel;

    channel = malloc(256 * 4096);
    printf("before: hit at %d\n", probe(channel,80, 4096));

    flush(channel, 4096);
    _mm_mfence();
    encode(channel,81, 4096);
    //maccess(channel + 'P' * 4096);
    _mm_mfence();
    printf("after: hit at %d\n", probe(channel,80, 4096));

    free(channel);
}