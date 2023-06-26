all: x86

x86: main_x86
arm: main_arm

main_x86: main.c
	gcc -static sidechannel/flush-reload/fr.c libcache/cache.c util.c pht.c main.c -o poc_x86 -Os -I libcache

main_arm: main.c
	aarch64-linux-gnu-gcc -march=armv8-a -D__ARM_ARCH_8A__ -static -Os sidechannel/flush-reload/fr.c libcache/cache.c util.c pht.c main.c -o poc_arm -I../../../ -static

prof_x86: main.c
	gcc -pg -static sidechannel/flush-reload/fr.c libcache/cache.c util.c pht.c main.c -o prof_x86 -Os -I libcache
	./prof_x86
	gprof ./prof_x86 gmon.out > results/profile.txt
	rm -f gmon.out

prof_arm: main.c
	aarch64-linux-gnu-gcc -pg -march=armv8-a -D__ARM_ARCH_8A__ -static -Os sidechannel/flush-reload/fr.c libcache/cache.c util.c pht.c main.c -o prof_arm -I../../../ -static
	./prof_arm
	gprof ./prof_x86 gmon.out > results/profile.txt

clean:
	rm -f poc_*
	rm -f prof_*
