all: x86

x86: btb_final.c
	make pkgbtb
	gcc -static btb_final.c ../sidechannel/flush_reload.o btb.o -o btb_x86

pkgbtb: btb.c
	gcc -static -c btb.c