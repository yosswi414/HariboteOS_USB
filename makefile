
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
CFLAGS_BASE = -march=i486 -m32 -fno-pie -fno-builtin -nostdlib -c
CFLAGS_O2 = -O2 $(CFLAGS_BASE)
CFLAGS_O0 = -O0 $(CFLAGS_BASE)
CFLAGS_SW = $(CFLAGS_O2)
DEL = rm -f
# デフォルト動作

.PHONY : default clean img evacuate mcp

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
	$(CC) $(CFLAGS_SW) $(KBD).c -o $@

$(MOU).obj : $(MOU).c device.h makefile
	$(CC) $(CFLAGS_SW) $(MOU).c -o $@

$(BTP).obj : $(BTP).c makefile
	$(CC) $(CFLAGS_SW) $(BTP).c -o $@

%.obj : %.c %.h makefile
	$(CC) $(CFLAGS_SW) $*.c -o $@

$(BTP).bin : $(OBJS) $(LKS) makefile
	ld -Map=$(BTP).map -m elf_i386 -T $(LKS) $(OBJS) -o $(BTP).bin
	
$(HRB) : $(ASH).bin $(BTP).bin makefile
	cat $(ASH).bin $(BTP).bin > $(HRB)

a_nasm.obj : a_nasm.asm makefile
	$(NASM) -f elf32 -o a_nasm.obj a_nasm.asm

crack1.hrb : crack1.c app.ld makefile
	$(CC) $(CFLAGS_SW) -o crack1.obj crack1.c
	ld -Map=crack1.map -m elf_i386 -T app.ld -o $@ crack1.obj

bugzero.hrb : bug_zerodiv.c mylibgcc.obj a_nasm.obj app.ld makefile
	$(CC) $(CFLAGS_SW) -o bug_zerodiv.obj bug_zerodiv.c
	ld -Map=bugzero.map -m elf_i386 -T app.ld -o $@ bug_zerodiv.obj mylibgcc.obj a_nasm.obj

%.hrb : %.asm app.ld makefile
	# $*
	$(NASM) -f elf32 -o $*.obj $*.asm -l $*.lst
	ld -Map=$*.map -m elf_i386 -T app.ld -o $@ $*.obj

# some illegal operation could be removed by optimization
%.hrb : %.c a_nasm.obj app.ld makefile
	$(CC) $(CFLAGS_SW) -o $*.obj -l $*.lst $*.c
	ld -Map=$*.map -m elf_i386 -T app.ld -o $@ $*.obj a_nasm.obj

FILES = $(HRB) walk.hrb lines.hrb stars2.hrb stars.hrb star1.hrb winhelo3.hrb winhlo2.hrb winhello.hrb hello5.hrb hello4.hrb bug3.hrb bug2.hrb bugzero.hrb bug1.hrb hello.hrb a.hrb helloapi.hrb crack1.hrb crack2.hrb crack3.hrb crack4.hrb crack5.hrb asmhead.asm fifo.c btp.ld readme.md window.h console.c Sarah_Crowely.txt
$(DST) : $(IPL) $(FILES) makefile
	mformat -f 1440 -C -B $(IPL) -i $@ ::
	$(foreach file, $(FILES), mcopy $(file) -i $@ ::;)	
###$(foreach file, $(FILES), $(eval $(call mcp,$(file))))
#dd if=$(IPL) of=$(DST)
#dd if=$(HRB) of=$(DST) seek=16896 oflag=seek_bytes ibs=512 conv=sync
#dd if=$(IPL) of=$(DST)
#dd if=$(HRB) of=$(DST) seek=16896 oflag=seek_bytes ibs=512 conv=sync
#$(foreach file, $(FILES_DD), dd if=$(file) of=$(DST) oflag=append ibs=512 conv=sync,notrunc;)

img :
	$(MAKE) $(DST)

clean :
	$(DEL) *.bin
	$(DEL) *.obj
	$(DEL) *.lst
	$(DEL) *.img
	$(DEL) *.sys
	$(DEL) *.map
	$(DEL) *.hrb

evacuate :
	git add ./*
	git commit -m 'in case of loss of data'
	git push origin master

.PHONY: test

define template
    @echo "arg : $1"

endef

test : $(HRB) bug1.hrb hello.hrb a.hrb helloapi.hrb crack1.hrb crack2.hrb crack3.hrb crack4.hrb crack5.hrb makefile
	$(foreach x,$^,$(call template,$(x)))