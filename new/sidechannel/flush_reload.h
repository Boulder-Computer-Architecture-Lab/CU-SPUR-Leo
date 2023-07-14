#ifndef _FLUSH_RELOAD_
#define _FLUSH_RELOAD_

#include <x86intrin.h>
#include <stdint.h>
#include <stdio.h>




inline void __attribute__((__always_inline__)) maccess(void *p) {asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }
int flush_reload_t(void *ptr);
void flush(char* oracle_ptr, size_t offset);

void probe(char* oracle_ptr, int thresh, size_t offset, int data[256]);
#endif