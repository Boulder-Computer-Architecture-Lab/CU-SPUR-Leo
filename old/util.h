#ifndef _B_UTIL_
#define _B_UTIL_

#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef linux
  #include <sched.h>
#endif

#include "libcache/cache.h"
#include "sidechannel/flush-reload/fr.h"

// accessible data
//#define DATA "data|"
#define DATA "d|"
// inaccessible secret (following accessible data)
//#define SECRET "INACCESSIBLE SECRET"
#define SECRET "SECR"
#define OVERWRITE   '#'
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define MAX_DATA 100

#define GEM5_COMPAT = 1

#define DATA_SECRET DATA SECRET

extern volatile int true;
extern unsigned char data[128];
extern char *mem;

// Span large part of memory with jump if equal
#if defined(__i386__) || defined(__x86_64__)
#define JE asm volatile("je end");
#elif defined(__aarch64__)
#define JE asm volatile("beq end");
#endif
#define JE_16 JE JE JE JE JE JE JE JE JE JE JE JE JE JE JE JE
#define JE_256 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16 JE_16
#define JE_4K JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256 JE_256
#define JE_64K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K JE_4K

typedef struct memSetup{
  char *_mem;
  char leaked[sizeof(DATA_SECRET)+1];
} memSetup __attribute__((aligned(8)));

void oop();

typedef struct runData{
  uint64_t op_times[MAX_DATA];
  int ops;
  uint64_t start;
  uint64_t end;
} runData;

void addOpData(runData *r);

void analyseOpData(runData *r, FILE* fptr);


// TODO: check size of data beforehand?
// access_array should not normally be able to access items outside of len
// but the spectre attack "allows us to"
char access_array(int x);

void setup(memSetup *m);

typedef struct config_t{
  int8_t attack_params;
  int verbosity;
} config_t;
// configuration parametres:
/*


  0: mistraining type: 
    0 for congruent branch ("oop"), 1 for victim/shadow branch ("ip")
  1: address space:
    0 for same address space ("sa"), 1 for cross-address space ("ca")
  3,2: target component:
    00 for BTB
    01 for PHT
    10 for RSB
    11 for STL (ignores 0,1)
  4: reserved
  6, 5: verbosity (helps with gem5)
    00 for no output
    01 for heartbeat every 10000 cycles
    10 for text representation
    11 for text representation and 

*/

#endif