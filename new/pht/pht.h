#ifndef _PHT_
#define _PHT_
#include <stdint.h>
#include <stdlib.h>
#include "../defines.h"


extern char* oracle_block;
extern char* victim_block;
extern unsigned int array1_size;

int pht_atk(size_t malicious_x);


#endif