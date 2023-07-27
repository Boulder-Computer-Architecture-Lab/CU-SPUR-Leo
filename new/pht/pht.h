#ifndef _PHT_
#define _PHT_
#include <stdint.h>
#include <stdlib.h>
#include <x86intrin.h>

#include "../sidechannel/flush_reload.h"
#include "../defines.h"


extern char* oracle_block; // external reference to sidechannel's oracle block
extern char* victim_block; // external reference to victim memory
extern unsigned int array1_size; // external reference to "safe" array size

void pht_atk(size_t malicious_x); // actual attack function wrapper
void victim_function(size_t x);  // vulnerable segment with a misspeculated bounds check


#endif