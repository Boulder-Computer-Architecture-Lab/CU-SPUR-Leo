#ifndef _PHT_GEN_
#define _PHT_GEN_
#include "util.h"

extern volatile int true;
extern unsigned char data[128];
extern size_t CACHE_MISS;
extern size_t pagesize;

void train_oop();

void train_ip();

void genPHTAtk(runData *r, memSetup *m, int *j);

char accessSTL(int x);

void sa(void (*trainer)(), runData *r, memSetup *m, int *j, pid_t pid);

void ca(void (*trainer)(), runData *r, memSetup *m, int *j, pid_t pid);

void stl(void (*trainer)(), runData *r, memSetup *m, int *j, pid_t pid);


#endif