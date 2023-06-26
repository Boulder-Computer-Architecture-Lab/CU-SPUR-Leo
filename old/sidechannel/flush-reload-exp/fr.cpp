#include "fr.hpp"


FlushReload::FlushReload(){
    probe_arr = nullptr;
    hit_table = nullptr;
}

void FlushReload::init(){
    probe_arr = new char[table_size * pagesize];
}