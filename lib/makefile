
MAKE	= make -r
DEL = rm -f
# -r (--no-builtin-rules) : eliminate use of the built-in implicit rules

NASM = nasm
INCLUDE = ../include
LIBRARY = -L.. -lapi -lmygcc
CC = gcc -I $(INCLUDE)
LINKER = ld
LINKER_L = $(LINKER) $(LIBRARY)

CFLAGS_ARCH = -march=i486 -m32 -fno-pie -fno-builtin -fno-delete-null-pointer-checks -nostdlib
CFLAGS_LIST = -Wa,-adhln -g
CFLAGS_BASE = $(CFLAGS_ARCH) $(CFLAGS_LIST) -c
CFLAGS_O2 = -O2 $(CFLAGS_BASE)
CFLAGS_O0 = -O0 $(CFLAGS_BASE)
CFLAGS_SW = $(CFLAGS_O2)

# APPS = walk.hrb lines.hrb stars2.hrb stars.hrb star1.hrb winhelo3.hrb winhlo2.hrb winhello.hrb hello5.hrb hello4.hrb bug3.hrb bug2.hrb bugzero.hrb bug1.hrb hello.hrb a.hrb helloapi.hrb crack1.hrb crack2.hrb crack3.hrb crack4.hrb crack5.hrb
# REQ = ../a_nasm.obj ../mylibgcc.obj
REQ = ../libapi.a ../libmygcc.a

clean :
	$(DEL) *.bin
	$(DEL) *.obj
	$(DEL) *.lst
	$(DEL) *.img
	$(DEL) *.sys
	$(DEL) *.map
	$(DEL) *.hrb
	$(DEL) *.vdi
	$(DEL) *.vmdk
	$(DEL) *.lib
	$(DEL) *.so
	$(DEL) *.a