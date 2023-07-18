all: x86

x86: rsb_final.c
	make pkgrsb
	gcc -static -Os rsb_final.c ../sidechannel/flush_reload.o rsb.o -o rsb_x86

pkgrsb: rsb.c
	gcc -static -Os -c rsb.c