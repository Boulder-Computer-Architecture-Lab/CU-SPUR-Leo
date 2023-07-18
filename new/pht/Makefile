all: x86

x86: pht_final.c
	
	gcc -static pht_final.c ../sidechannel/flush_reload.o -o pht_x86

pkgpht: pht.c
	gcc -static -c pht.c