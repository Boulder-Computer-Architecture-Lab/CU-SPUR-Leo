#ifndef SIDECHANNEL
#define SIDECHANNEL

void cache_encode(char data);
void cache_decode(char *leaked, int index);
void cache_decode_pretty(char *leaked, int index);


#endif