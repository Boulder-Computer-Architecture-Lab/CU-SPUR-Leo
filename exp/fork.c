#include <stdio.h>
#include <sched.h>


// experimenting with forking a process

void main(){

    pid_t pid;
    pid = fork();
    if (pid != 0){
        printf("another process");
    }
}