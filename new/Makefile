all: btb pht rsb x86


x86: main.c
	gcc -static util.c main.c rsb/rsb.o btb/btb.o pht/pht.o sidechannel/flush_reload.o -o test.out

btb: btb/btb.c btb/btb.h btb/btb_final.c
	cd btb && make

pht: pht/pht.c pht/pht.h pht/pht_final.c
	cd pht && make

rsb: rsb/rsb.c rsb/rsb.h rsb/rsb_final.c
	cd rsb && make
sidechannel: sidechannel/flush_reload.h sidechannel/flush_reload.c
	cd sidechannel && make