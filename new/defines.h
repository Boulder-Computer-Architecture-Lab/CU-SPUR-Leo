#ifndef DEFINES
#define DEFINES

// test codes
#define PHT_CODE 0b001
#define BTB_CODE 0b010
#define RSB_CODE 0b100

// user options
#define CACHE_MISS 80
#define PAGESIZE 4096
#define SECRET "foobar"
#define TRIES 5 // number of tries for each character: reducing may speed program up

// command defaults
#define TESTS 0
#define TIME_EX 0
#define FILENAME 0

// test parameters
#define BTB_TRAIN_RUNS 30
#define PHT_ATTEMPTS 30
#define PHT_SAFEPERMAL 5 // number of iterations between attempts


#endif