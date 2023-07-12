#pragma once
#define _FLUSH_RELOAD_

#include <x86intrin.h>
#include <stdint.h>
#include <stdio.h>

typedef struct fr_channel{
    char* oracle_block;
    size_t offset;
    int results[256];
} fr_channel;

void make_channel(fr_channel *channel);
void free_channel(fr_channel *channel);


int flush_reload_t(void *ptr);
void flush(fr_channel *channel);

int probe(fr_channel *channel, int thresh);
void encode(fr_channel *channel, char data);
