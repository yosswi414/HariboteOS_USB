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

OBJS = $(BTP).obj $(FNC).obj $(LIB).obj $(DSC).obj $(GRP).obj $(INT).obj $(QUE).obj $(KBD).obj $(MOU).obj $(MEM).obj font.obj
CFLAGS = -O2 -march=i486 -m32 -fno-pie -nostdlib -c

DEL = rm -f
# デフォルト動作

default :
	#$(MAKE) clean
	$(MAKE) img

# ファイル生成規則

$(IPL) : ipl.asm makefile
	$(NASM) -o $(IPL) ipl.asm -l ipl.lst

$(ASH).bin : $(ASH).asm makefile
	$(NASM) -o $(ASH).bin $(ASH).asm -l $(ASH).lst

$(FNC).obj : $(FNC).asm makefile
	$(NASM) -f elf -o $(FNC).obj $(FNC).asm -l $(FNC).lst

font.obj: hankaku.txt makefile
	../HariboteOS_img/tolset/z_tools/makefont.exe hankaku.txt font.bin
	objcopy -I binary -O elf32-i386 -B i386 --redefine-sym _binary_font_bin_start=ascii font.bin font.obj

%.obj : %.c makefile
	$(CC) $(CFLAGS) $*.c -o $*.obj

$(BTP).bin : $(OBJS) $(LKS) makefile
	ld -m elf_i386 -T $(LKS) $(OBJS) -o $(BTP).bin
	
$(HRB) : $(ASH).bin $(BTP).bin makefile
	cat $(ASH).bin $(BTP).bin > $(HRB)

$(DST) : $(IPL) $(HRB) makefile
	dd if=$(IPL) of=$(DST)
	dd if=$(HRB) of=$(DST) seek=16896 oflag=seek_bytes ibs=512 conv=sync

img :
	$(MAKE) $(DST)

clean :
	$(DEL) *.bin
	$(DEL) *.obj
	$(DEL) *.lst
	$(DEL) *.img
	$(DEL) *.sys