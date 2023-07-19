#include "util.h"

static char doc[] = "spectre benchmarks for gem5 and linux";

static struct argp_option options[] = {
    {"use-file",  'f', "FILE",      OPTION_ARG_OPTIONAL,  "use file <FILE> (if not provided, defaults to spec_bench.txt) to log output" },
    {"time-ex",    't', 0,      0,  "time extraction for individual characters" },
    {"pht",   'p', 0,           0, "run pht test. if no explicit test flags are set, all tests are run." },
    {"btb",   'b', 0, 0, "run btb test. if no explicit test flags are set, all tests are run." },
    {"rsb",   'r', 0, 0, "run rsb test. if no explicit test flags are set, all tests are run." },
    { 0 },
};

static char args_doc[] = "ARG1 ARG2";

void prepareArgs (struct args *a){
    a->time_extract = 0;
    a->tests_to_run = 0;
    a->filename = 0;
}

error_t parse_opt(int key, char *arg, struct argp_state *state){
   struct args *a = state->input;
    switch (key)
    {
    case 't': 
        a->time_extract = 1; 
        break;
    case 'p':
      a->tests_to_run |= 1;
      break;
    case 'b':
        a->tests_to_run |= 0b10;
        break;
    case 'r':
        a->tests_to_run |= 0b100;
        break;
    case 'f':
        if (arg != 0){
            a->filename = arg;
        } else{
            a->filename = "spec_bench.txt";
        }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
static struct argp argp = {options, parse_opt,args_doc, doc};

error_t parse(int argc, char **argv, struct args *a){
    prepareArgs(a);
    return argp_parse(&argp,argc,argv,0,0,a);
}