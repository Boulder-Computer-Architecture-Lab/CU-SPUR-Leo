#include "flush_reload.h"
#include <stdio.h>

int main(){
    fr_channel *channel;
    channel = malloc(sizeof(fr_channel));

    make_channel(channel);
    printf("before: hit at %d\n", probe(channel,80));

    flush(channel);
    _mm_mfence();
    encode(channel,'P');
    _mm_mfence();
    printf("after: hit at %d\n", probe(channel,80));

    free_channel(channel);
    free(channel);
}