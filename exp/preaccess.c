#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>

/* 
    verifying that pre-accessing the pointer of a variable reduces the
    load time => successful
*/
char secret = 'S';
int junk = 0;

void __attribute__((__always_inline__)) maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

int flush_reload_t(void *ptr) {
  register uint64_t start = 0, end = 0;
  start = __rdtscp(&junk);
  maccess(ptr);

  end = __rdtscp(&junk);
  _mm_mfence();
  //flush(ptr);
  _mm_clflush(ptr);

  return (int)(end - start);
}

int main(){
    _mm_clflush(&secret);
    printf("initial: %d\n", flush_reload_t(&secret));
    printf("initial: %d\n", flush_reload_t(&secret));

    maccess(&secret);

    printf("post-access: %d\n", flush_reload_t(&secret));
    printf("post-access: %d\n", flush_reload_t(&secret));
}