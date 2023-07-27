#ifndef _RSB_
#define _RSB_

#include <stdint.h>
#include "../defines.h"
#include "../sidechannel/flush_reload.h"

extern char* oracle_block; // external reference to sidechannel's oracle block


int __attribute__ ((noinline)) call_manipulate_stack(); // remove top of stack, flush rsp to delay return
int __attribute__ ((noinline)) call_leak(char s); // the leaky function
int __attribute__ ((noinline)) call_start(char s); // start call stack
void confuse_compiler(); // function so that above is compiled into functions rather than jumps
void rsb_atk(char info); // actual attack function wrapper



#endif