#ifndef FLUSH_RELOAD
#define FLUSH_RELOAD

#include "../sidechannel.hpp"
#include <stdio.h>

extern C{
    #include "../../util.h"
}

class FlushReload : public Sidechannel{
    private:
        char* probe_arr;
        int* hit_table;
        static const int table_size = 256;
        static const int hit_thresh = 100;
    public:
        FlushReload();
        void init(bool initHit);
};


#endif