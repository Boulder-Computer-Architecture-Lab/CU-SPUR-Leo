all: x86

x86: btb-scratch.c
	make pkgbtb
	gcc -static btb-scratch.c ../sidechannel/flush_reload.o btb.o -o final.out

pkgbtb: btb.c
	gcc -static -c btb.c