#ifndef _PHT_
#define _PHT_
#include <stdint.h>
#include <stdlib.h>
#include <x86intrin.h>

#include "../sidechannel/flush_reload.h"
#include "../defines.h"


extern char* oracle_block;
extern char* victim_block;
extern unsigned int array1_size;

void pht_atk(size_t malicious_x);
void victim_function(size_t x);


#endif