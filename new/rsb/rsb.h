#ifndef _RSB_
#define _RSB_

#include <stdint.h>
#include "../defines.h"
#include "../sidechannel/flush_reload.h"

extern char* oracle_block;


int __attribute__ ((noinline)) call_manipulate_stack();
int __attribute__ ((noinline)) call_leak(char s);
int __attribute__ ((noinline)) call_start(char s);
void confuse_compiler();
void rsb_atk(char info);



#endif