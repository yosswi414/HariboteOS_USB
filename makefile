
MAKE	= make -r
DEL = rm -f
# -r (--no-builtin-rules) : eliminate use of the built-in implicit rules
INCLUDE = ./include
LIBRARY = -lapi -lmygcc
ASH	= asmhead
BTP	= bootpack
IPL	= ipl.bin
FNC	= naskfunc
HRB = haribote.sys
DST = usbboot.img
NASM = nasm
CC = gcc -I $(INCLUDE) -Wall
# CC = $(CC_L) $(LIBRARY)
LINKER = ld $(LIBRARY)
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
ACPI = acpi
SYS = sysfunc

PROG_CHPT = 28
PROG_PAGE = 602

OBJS = $(SYS).obj $(ACPI).obj $(FIL).obj $(CON).obj $(MUL).obj $(TIM).obj $(WND).obj $(SHT).obj $(BTP).obj $(FNC).obj $(DSC).obj $(GRP).obj $(INT).obj $(QUE).obj $(KBD).obj $(MOU).obj $(MEM).obj font.obj
CFLAGS_ARCH = -march=i486 -m32 -fno-pie -fno-builtin -fno-delete-null-pointer-checks -nostdlib
CFLAGS_LIST = -Wa,-adhln -g
CFLAGS_BASE = $(CFLAGS_ARCH) $(CFLAGS_LIST) -c
CFLAGS_O2 = -O2 $(CFLAGS_BASE)
CFLAGS_O0 = -O0 $(CFLAGS_BASE)
CFLAGS_SW = $(CFLAGS_O2)

# デフォルト動作

.PHONY : default clean img evacuate mcp

default :
	$(MAKE) img


VBM = vboxmanage
VDI = usbboot.vdi
VM_NAME = HariboteOS_USB
VM_SCTL = SATA
VM_UUID := 0

run : $(VDI)
	$(VBM) startvm "HariboteOS_USB"

$(VDI) : img
ifndef $(type $(VBM) > /dev/null 2>&1; echo $?)
	$(eval VBM := vboxmanage.exe)
endif
	-mv $(VDI) $(VDI).bak
	-mv usbboot.vmdk usbboot.vmdk.bak
	$(VBM) convertfromraw -format VDI $(DST) $(VDI)
	$(VBM) convertfromraw -format VMDK $(DST) usbboot.vmdk
	$(eval VM_UUID := $(shell $(VBM) list hdds | grep -e "^UUID" | awk '{print $$2}'))
	# $(VBM) storagectl $(VM_NAME) --name $(VM_SCTL) --portcount 1 --remove
	$(VBM) internalcommands sethduuid $(VDI) $(VM_UUID)
	# -$(VBM) setextradata $(VM_NAME) "VBoxInternal/Devices/i8254/0/Config/PassthroughSpeaker" 100
	-$(VBM) storagectl $(VM_NAME) --name $(VM_SCTL) --add sata --controller IntelAHCI --portcount 1 --bootable on
	-$(VBM) storageattach $(VM_NAME) --storagectl $(VM_SCTL) --device 0 --port 0 --type hdd --medium $(VDI)
	cp $(VDI) /mnt/c/Users/yossw/Documents/img/
	cp usbboot.vmdk /mnt/c/Users/yossw/Documents/img/

# ファイル生成規則

$(IPL) : ipl.asm makefile
	$(NASM) -o $@ ipl.asm -l ipl.lst

$(ASH).bin : $(ASH).asm makefile
	$(NASM) -o $@ $(ASH).asm -l $(ASH).lst

$(FNC).obj : $(FNC).asm makefile
	$(NASM) -f elf -o $@ $(FNC).asm -l $(FNC).lst

font.obj: hankaku.txt makefile
	z_tools/makefont.exe hankaku.txt font.bin
	objcopy -I binary -O elf32-i386 -B i386 --redefine-sym _binary_font_bin_start=ascii font.bin $@

$(KBD).obj : $(KBD).c $(INCLUDE)/device.h makefile
	$(CC) $(CFLAGS_SW) $(KBD).c -o $@ > $(KBD).lst

$(MOU).obj : $(MOU).c $(INCLUDE)/device.h makefile
	$(CC) $(CFLAGS_SW) $(MOU).c -o $@ > $(MOU).lst

$(BTP).obj : $(BTP).c makefile
	$(CC) $(CFLAGS_SW) $(BTP).c -o $@ \
		-D'PROGRESS_CHAPTER=$(PROG_CHPT)' \
		-D'PROGRESS_PAGE=$(PROG_PAGE)' \
		-D'PROGRESS_YEAR="$(shell date '+%Y')"'  \
		-D'PROGRESS_MONTH="$(shell date '+%m')"' \
		-D'PROGRESS_DAY="$(shell date '+%d')"' \
		-D'PROGRESS_HOUR="$(shell date '+%H')"' \
		-D'PROGRESS_MIN="$(shell date '+%M')"' \
		> $(BTP).lst

# a_nasm.obj : a_nasm.asm makefile
# 	$(NASM) -f elf32 -o a_nasm.obj a_nasm.asm -l a_nasm.lst

%.obj : %.asm makefile
	$(NASM) -f elf32 -o $@ $*.asm -l $*.lst

%.obj : %.c $(INCLUDE)/%.h makefile
	$(CC) $(CFLAGS_SW) $*.c -o $@ > $*.lst

%.obj : %.c $(INCLUDE)/$(LIB).h makefile
	$(CC) $(CFLAGS_SW) $*.c -o $@ > $*.lst

$(BTP).bin : $(OBJS) $(LKS) libmygcc.a makefile
	ld -Map=$(BTP).map -m elf_i386 -T $(LKS) $(OBJS) libmygcc.a -o $(BTP).bin
	
$(HRB) : $(ASH).bin $(BTP).bin makefile
	cat $(ASH).bin $(BTP).bin > $(HRB)

CORE = $(HRB) 
FILES = asmhead.asm btp.ld README.md $(INCLUDE)/window.h console.c Sarah_Crowely.txt builder.txt
CONTENT = $(CORE) $(FILES)

# APPS = walk.hrb lines.hrb stars2.hrb stars.hrb star1.hrb winhelo3.hrb winhlo2.hrb winhello.hrb hello5.hrb hello4.hrb bug3.hrb bug2.hrb bugzero.hrb bug1.hrb hello.hrb a.hrb helloapi.hrb crack1.hrb crack2.hrb crack3.hrb crack4.hrb crack5.hrb
APPS = cat.hrb catipl.hrb prime.hrb color2.hrb color.hrb beepdown.hrb noodle.hrb walk.hrb lines.hrb stars2.hrb stars.hrb star1.hrb winhelo3.hrb winhlo2.hrb winhello.hrb hello5.hrb hello4.hrb hello.hrb helloapi.hrb
OBJS_API = api026.obj api021.obj api022.obj api023.obj api024.obj api025.obj api001.obj api002.obj api003.obj api004.obj api005.obj api006.obj api007.obj api008.obj api009.obj api010.obj api011.obj api012.obj api013.obj api014.obj api015.obj api016.obj api017.obj api018.obj api019.obj api020.obj
OBJS_APIS = $(addprefix api/,$(OBJS_API))

REQ = libapi.a libmygcc.a

libapi.a : $(OBJS_APIS) makefile
	ar rcs $@ $(OBJS_APIS)
	# $(CC) $(CFLAGS_ARCH) -o $@ $(OBJS_APIS)

MLG_012 = mlg_012_1.obj mlg_012_2.obj mlg_012_3.obj mlg_012_4.obj mlg_012_5.obj mlg_012_6.obj mlg_012_7.obj mlg_012_8.obj mlg_012_9.obj
OBJS_MLG = mlg_001.obj mlg_002.obj mlg_003.obj mlg_004.obj mlg_005.obj mlg_006.obj mlg_007.obj mlg_008.obj mlg_009.obj mlg_010.obj mlg_011.obj $(MLG_012) mlg_013.obj mlg_014.obj mlg_015.obj 
OBJS_MLGS = $(addprefix lib/,$(OBJS_MLG))

libmygcc.a : $(OBJS_MLGS) makefile
	ar rcs $@ $(OBJS_MLGS)

# libmygcc.a : $(LIB).obj makefile
	# ar rcs $@ $(LIB).obj
	# $(CC) $(CFLAGS_ARCH) -o $@ mylibgcc.obj

# REQ = a_nasm.obj mylibgcc.obj
# REQ = $(OBJS_APIS) mylibgcc.obj


builder.txt :
	echo '[$$USER]' > $@
	echo ${USER} >> $@
	echo "[hostname]" >> $@
	echo `hostname` >> $@
	echo "[date]" >> $@
	echo `date` >> $@
	echo "[uname -a]" >> $@
	echo `uname -a` >> $@
	echo "[cat /proc/version]" >> $@
	echo `cat /proc/version` >> $@
	echo '[bash --version]' >> $@
	echo `bash --version` >> $@
	echo "[gcc --version]" >> $@
	echo `gcc --version` >> $@
	echo "[make --version]" >> $@
	echo `make --version` >> $@
	

%.hrb : apps/makefile $(REQ) makefile
	(cd apps; $(MAKE) $@)

$(DST) : $(IPL) $(CONTENT) $(APPS) makefile
	mformat -f 1440 -C -B $(IPL) -i $@ ::
	$(foreach file, $(CONTENT), mcopy $(file) -i $@ ::;)
	$(foreach file, $(APPS), mcopy apps/$(file) -i $@ ::;)
###$(foreach file, $(FILES), $(eval $(call mcp,$(file))))
#dd if=$(IPL) of=$(DST)
#dd if=$(HRB) of=$(DST) seek=16896 oflag=seek_bytes ibs=512 conv=sync
#dd if=$(IPL) of=$(DST)
#dd if=$(HRB) of=$(DST) seek=16896 oflag=seek_bytes ibs=512 conv=sync
#$(foreach file, $(FILES_DD), dd if=$(file) of=$(DST) oflag=append ibs=512 conv=sync,notrunc;)

img : $(REQ)
	$(MAKE) $(DST)
	cp $(DST) /mnt/c/Users/yossw/Documents/img/
	$(DEL) builder.txt

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
	(cd apps; $(MAKE) clean)

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
