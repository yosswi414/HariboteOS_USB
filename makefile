MAKE	= make -r
# -r (--no-builtin-rules) : eliminate use of the built-in implicit rules
ASH	= asmhead
BTP	= bootpack
IPL	= ipl.bin
FNC	= naskfunc
HRB = haribote.sys
DST = usbboot.img
NASM = nasm
CC = gcc
LIB = mylibgcc
DSC = desctable
GRP = graphic
LKS = btp.ld
INT = interrupt
QUE = fifo
KBD = keyboard
MOU = mouse
MEM = memory
SHT = sheet
WND = window
TIM = timer
MUL = mtask
CON = console
FIL = file

OBJS = $(FIL).obj $(CON).obj $(MUL).obj $(TIM).obj $(WND).obj $(SHT).obj $(BTP).obj $(FNC).obj $(LIB).obj $(DSC).obj $(GRP).obj $(INT).obj $(QUE).obj $(KBD).obj $(MOU).obj $(MEM).obj font.obj
CFLAGS = -O2 -march=i486 -m32 -fno-pie -fno-builtin -nostdlib -c

DEL = rm -f
# デフォルト動作

default :
	#$(MAKE) clean
	$(MAKE) img

# ファイル生成規則

$(IPL) : ipl.asm makefile
	$(NASM) -o $@ ipl.asm -l ipl.lst

$(ASH).bin : $(ASH).asm makefile
	$(NASM) -o $@ $(ASH).asm -l $(ASH).lst

$(FNC).obj : $(FNC).asm makefile
	$(NASM) -f elf -o $@ $(FNC).asm -l $(FNC).lst

font.obj: hankaku.txt makefile
	../HariboteOS_img/tolset/z_tools/makefont.exe hankaku.txt font.bin
	objcopy -I binary -O elf32-i386 -B i386 --redefine-sym _binary_font_bin_start=ascii font.bin $@

$(KBD).obj : $(KBD).c device.h makefile
	$(CC) $(CFLAGS) $(KBD).c -o $@

$(MOU).obj : $(MOU).c device.h makefile
	$(CC) $(CFLAGS) $(MOU).c -o $@

$(BTP).obj : $(BTP).c makefile
	$(CC) $(CFLAGS) $(BTP).c -o $@

%.obj : %.c %.h makefile
	$(CC) $(CFLAGS) $*.c -o $@

$(BTP).bin : $(OBJS) $(LKS) makefile
	ld -Map=$(BTP).map -m elf_i386 -T $(LKS) $(OBJS) -o $(BTP).bin
	
$(HRB) : $(ASH).bin $(BTP).bin makefile
	cat $(ASH).bin $(BTP).bin > $(HRB)

hello.hrb : hello.asm makefile
	$(NASM) -o $@ hello.asm -l hello.lst

a_nasm.obj : a_nasm.asm makefile
	$(NASM) -f elf32 -o a_nasm.obj a_nasm.asm

a.hrb : a.c a_nasm.obj app.ld makefile
	$(CC) $(CFLAGS) -o a.obj a.c
	ld -Map=a.map -m elf_i386 -T app.ld -o $@ a.obj a_nasm.obj

helloapi.hrb : hello_api.c a_nasm.obj app.ld makefile
	$(CC) $(CFLAGS) -o hello_api.obj hello_api.c
	ld -Map=a.map -m elf_i386 -T app.ld -o $@ hello_api.obj a_nasm.obj

crack1.hrb : crack1.c app.ld makefile
	$(CC) $(CFLAGS) -o crack1.obj crack1.c
	ld -Map=crack1.map -m elf_i386 -T app.ld -o $@ crack1.obj
crack2.hrb : crack2.asm makefile
	$(NASM) -o $@ crack2.asm -l crack2.lst

$(DST) : $(IPL) $(HRB) hello.hrb a.hrb helloapi.hrb crack1.hrb crack2.hrb makefile
	mformat -f 1440 -C -B $(IPL) -i $@ ::
	mcopy $(HRB) -i $@ ::
	mcopy asmhead.asm -i $@ ::
	mcopy fifo.c -i $@ ::
	mcopy btp.ld -i $@ ::
	mcopy readme.md -i $@ ::
	mcopy window.h -i $@ ::
	mcopy console.c -i $@ ::
	mcopy hello.hrb -i $@ ::
	mcopy a.hrb -i $@ ::
	mcopy helloapi.hrb -i $@ ::
	mcopy crack1.hrb -i $@ ::
	mcopy crack2.hrb -i $@ ::
	mcopy Sarah_Crowely.txt -i $@ ::
	#dd if=$(IPL) of=$(DST)
	#dd if=$(HRB) of=$(DST) seek=16896 oflag=seek_bytes ibs=512 conv=sync

img :
	$(MAKE) $(DST)

clean :
	$(DEL) *.bin
	$(DEL) *.obj
	$(DEL) *.lst
	$(DEL) *.img
	$(DEL) *.sys
	$(DEL) *.map

evacuate :
	git add ./*
	git commit -m 'in case of loss of data'
	git push origin master
