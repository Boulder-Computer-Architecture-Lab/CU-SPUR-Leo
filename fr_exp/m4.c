#include <stdio.h>
#include <sched.h>


void main(){

    pid_t pid;
    pid = fork();
    if (pid != 0){
        printf("another process");
    }
}