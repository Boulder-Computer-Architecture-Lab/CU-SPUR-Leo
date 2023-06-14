all: x86

x86: main_x86
arm: main_arm

main_x86: ca.c
	gcc libcache/cache.c util.c pht.c ca.c -o poc_x86 -Os -I libcache

main_arm: ca.c
	aarch64-linux-gnu-gcc -march=armv8-a -D__ARM_ARCH_8A__ -static -Os libcache/cache.c util.c pht.c ca.c -o poc_arm -I../../../

prof_x86: ca.c
	gcc -pg libcache/cache.c util.c pht.c ca.c -o poc_x86 -Os -I libcache

prof_arm: ca.c
	aarch64-linux-gnu-gcc -pg -march=armv8-a -D__ARM_ARCH_8A__ -static -Os libcache/cache.c util.c pht.c ca.c -o poc_arm -I../../../

clean:
	rm -f poc_*
