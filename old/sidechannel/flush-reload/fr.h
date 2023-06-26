#ifndef FLUSH_RELOAD
#define FLUSH_RELOAD

#include "../../libcache/cache.h"
#include <x86intrin.h>

void cache_encode(char data);
void cache_decode(char *leaked, int index);
void cache_decode_pretty(char *leaked, int index);

size_t detect_flush_reload_threshold();
int flush_reload(void *ptr);
int flush_reload_t(void *ptr);




#endif