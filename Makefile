all: x86

x86: main_x86
arm: main_arm

main_x86: ca.c
	gcc util.h ca.c -o poc_x86 -Os -I libcache

main_arm: ca.c
	aarch64-linux-gnu-gcc -march=armv8-a -D__ARM_ARCH_8A__ -static -Os util.h ca.c -o poc_arm -I../../../

clean:
	rm -f poc_*
