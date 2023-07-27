#ifndef _UTIL_
#define _UTIL_

#include <argp.h>
#include <stdint.h>

#include "defines.h"

/**
 * test_to_run specification
 * bit position 0: pht
 * bit position 1: btb
 * bit position 2: rsb
**/

struct args{
    uint8_t time_extract; // whether or not to time extraction process for individual characters
    uint8_t tests_to_run; 
    char* filename;
};





void prepareArgs(struct args *a);

error_t parse_opt(int key, char *arg, struct argp_state *state);

error_t parse(int argc, char **argv, struct args *a);



#endif