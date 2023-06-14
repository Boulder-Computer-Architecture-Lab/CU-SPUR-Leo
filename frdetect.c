#include <stdio.h>
#include <stdint.h>

__attribute__((__always_inline__)) uint64_t rdtsc() {
  uint64_t a, d;
  asm volatile("mfence");
  asm volatile("rdtsc" : "=a"(a), "=d"(d));
  a = (d << 32) | a;
  asm volatile("mfence");
  return a;
}

// ---------------------------------------------------------------------------
__attribute__((__always_inline__)) int flush_reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

  start = rdtsc();
  maccess(ptr);
  end = rdtsc();

  mfence();

  flush(ptr);

  return (int)(end - start);
}

// ---------------------------------------------------------------------------
__attribute__((__always_inline__)) int reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

  start = rdtsc();
  maccess(ptr);
  end = rdtsc();

  mfence();

  return (int)(end - start);
}

__attribute__((__always_inline__)) void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }
__attribute__((__always_inline__)) void mfence() { asm volatile("mfence"); }
__attribute__((__always_inline__)) void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }


__attribute__((__always_inline__)) size_t detect_flush_reload_threshold() {
  size_t reload_time = 0, flush_reload_time = 0, i, count = 1000000;
  size_t dummy[16];
  size_t *ptr = dummy + 8;

  maccess(ptr);
  for (i = 0; i < count; i++) {
    reload_time += reload_t(ptr);
  }
  for (i = 0; i < count; i++) {
    flush_reload_time += flush_reload_t(ptr);
  }
  reload_time /= count;
  flush_reload_time /= count;

  return (flush_reload_time + reload_time * 2) / 3;
}

int main(){
  //printf("%d\n",detect_flush_reload_threshold());
  uint64_t a = rdtsc();
  printf("%d \n", a);
}