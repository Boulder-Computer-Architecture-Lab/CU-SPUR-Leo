all: x86

x86: pht_final.c
	make pkgpht
	gcc -static pht_final.c ../sidechannel/flush_reload.o pht.o -o pht_x86

pkgpht: pht.c
	gcc -static -c pht.c