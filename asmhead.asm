
VBEMODE	EQU		0x105
	; VESA VBE video modes
	; http://www.faqs.org/faqs/pc-hardware-faq/supervga-programming/
	;0x101				; 640 x 480 x 8bit color
	;0x103				; 800 x 600 x 8bit color
	;0x105				; 1024 x 768 x 8bit color
	;0x107				; 1280 x 1024 x 8bit color
	;0x11B				; 1280 x 1024 x 32bit color

BOTPAK	EQU		0x00280000		; where bootpack will be loaded
DSKCAC	EQU		0x00100000		; address of disk cache
DSKCAC0	EQU		0x00080000		; address of disk cache (real mode)

; BOOT_INFO
CYLS	EQU		0x0ff0			; set by boot sector
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; depth of colors
SCRNX	EQU		0x0ff4			; screen x
SCRNY	EQU		0x0ff6			; screen x
VRAM	EQU		0x0ff8			; initial point of graphic buffer

	ORG		0xc200

; check if VBE is available
	MOV		AX, 0x9000
	MOV		ES, AX
	MOV		DI, 0
	MOV		AX, 0x4f00
	INT		0x10
	CMP		AX, 0x004f
	JNE		scrn320

; check VBE version
	MOV		AX, [ES:DI+4]
	CMP		AX, 0x0200
	JB		scrn320				; if (AX < 0x0200) goto scrn320;

; get video mode information
	MOV		CX, VBEMODE
	MOV		AX, 0x4f01
	INT		0x10
	CMP		AX, 0x004f
	JNE		scrn320
	
; verify video mode information
	CMP		BYTE [ES:DI + 0x19], 8	; 8: number of colors
	JNE		scrn320

	CMP		BYTE [ES:DI + 0x1b], 4	; 4: palette mode
	JNE		scrn320

	MOV		AX, [ES:DI + 0x00]
	AND		AX, 0x0080			; bit-7 should be 1
	JZ		scrn320

; switch video mode
	MOV		BX, VBEMODE + 0x4000
	MOV		AX, 0x4f02
	INT		0x10

; save video mode
	MOV		BYTE [VMODE], 8
	MOV		AX, [ES:DI + 0x12]
	MOV		[SCRNX], AX
	MOV		AX, [ES:DI + 0x14]
	MOV		[SCRNY], AX
	MOV		EAX, [ES:DI + 0x28]
	MOV		[VRAM], EAX
	JMP		keystatus

scrn320:
	MOV		AX, 0x0013			; VGA graphics, 320 x 200 x 8bit color
	INT		0x10
	MOV		BYTE [VMODE], 8
	MOV		WORD [SCRNX], 320
	MOV		WORD [SCRNY], 200
	MOV		DWORD [VRAM], 0x000a0000

; get keyboard LED state
keystatus:
	MOV		AH, 0x02
	INT		0x16				; AH=0x02 : get keylock & shift state
	MOV		[LEDS], AL			; AL <- state code

; according to specification of AT compatibles, when PIC is initialized, 
; it has to prevent PIC from receiving any interruption before CLI
; or it may cause hang-up
; PIC should be initialized later

	MOV		AL, 0xff
	OUT		0x21, AL			; io_out(PIC0_IMR, 0xff);
	NOP							; consecutive OUT instr will not work with some machines
	OUT		0xa1, AL			; io_out(PIC1_IMR, 0xff); 
	
	CLI							; disallow interruption in CPU level

; configure A20GATE in order to let CPU access over 1MB of RAM
	
	CALL	waitkbdout
	MOV		AL, 0xd1
	OUT		0x64, AL
	CALL	waitkbdout
	MOV		AL, 0xdf			; enable A20
	OUT		0x60, AL
	CALL	waitkbdout

; transition to protect mode

	LGDT	[GDTR0]				; configure transitional GDT
	MOV		EAX, CR0			; Controll Register 0 : user should not access
	AND		EAX, 0x7fffffff		; reset bit31 in order to forbid paging
	OR		EAX, 0x00000001		; set bit0 in order to transit to protect mode
								; (protect mode: Protected Virtual Address Mode)
								; (real mode: Real Address Mode)
	MOV		CR0, EAX			
	JMP		pipelineflush		; this JMP cannot be omitted
								; mode has changed and CPU should re-do instr in pipeline
								; so where to go has no meaning while JMP itself DOES have
	
pipelineflush:
	MOV		AX, 1*8				; re-writable segment 32bit
	MOV		DS, AX				; leave CS for now
	MOV		ES, AX
	MOV		FS, AX
	MOV		GS, AX
	MOV		SS, AX
	
; transfer bootpack
	MOV		ESI, bootpack		; source
	MOV		EDI, BOTPAK			; destination
	MOV		ECX, 512*1024/4		; copy 512 KB
	CALL	memcpy

; transfer diskdata to its original position

; transferring boot sector
	MOV		ESI, 0x7c00			; source
	MOV		EDI, DSKCAC			; destination
	MOV		ECX, 512/4			; copy 512 B = 1 sector
	CALL	memcpy
	
; transferring the other part
	MOV		ESI, DSKCAC0+512	; source
	MOV		EDI, DSKCAC+512		; destination
	MOV		ECX, 0
	MOV		CL, BYTE [CYLS]
	IMUL	ECX, 512*18*2/4		; convert cylinder to byte/4
	SUB		ECX, 512/4			; subtract the size of IPL
	CALL	memcpy

; above all, asmhead finishes its job and leave the rest up to bootpack

; launching bootpack
	MOV		EBX, BOTPAK
	MOV		ECX, [EBX+16]		; copy ceil([EBX+16]/4) B
	ADD		ECX, 3				; ECX += 3
	SHR		ECX, 2				; ECX /= 4
	JZ		skip				; if there is nothing to transfer
	MOV		ESI, [EBX+20]		; source
	ADD		ESI, EBX
	MOV		EDI, [EBX+12]		; destination
	CALL	memcpy

skip:
	MOV		ESP, [EBX+12]		; initial value of stack
	JMP		DWORD	2*8:0x0000001b	; 0x1b in (2nd segment = bootpack.bin)

waitkbdout:
	IN		AL, 0x64
	AND		AL, 0x02
	IN		AL, 0x60			; flush buffer in case data exists
	JNZ		waitkbdout			; if res != 0 continue
	RET

memcpy:
	MOV		EAX, [ESI]
	ADD		ESI, 4
	MOV		[EDI], EAX
	ADD		EDI, 4
	SUB		ECX, 1
	JNZ		memcpy				; if res != 0 continue
	RET
; note: with address size prefixes, it can be written with string instr

	ALIGN	16,	DB	0

GDT0:
	TIMES	8	DB	0			; null selector
	DW		0xffff, 0x0000, 0x9200, 0x00cf	; re-writable segment 32 bit
	DW		0xffff, 0x0000, 0x9a28, 0x0047	; executable segment 32 bit (for bootpack)
	DW		0
	
GDTR0:
	DW		8*3-1				; limit
	DD		GDT0				; GDT begins from GDT0
	
	ALIGNB	16,	DB	0

ending:
	MOV		SI, msg_end
	JMP		putloop_e
fin:
	HLT
	JMP		fin
putloop_e:
	MOV		AL,[SI]
	ADD		SI,1				; ++SI
	CMP		AL,0
	JE		fin
	MOV		AH,0x0e
	MOV		BX, [chclr]			; BX : color code
	INT		0x10				; AH=0x0e : putchar()
	JMP		putloop_e
msg_end:
	;DB		0x0d, 0x0a			; CR LF
	DB		"Congrats! The pr"
	DB		"ogram has been l"
	DB		"oaded."
	DB		0x0d, 0x0a			; CR LF
	DB		"v:2020_0212_0000"
	DB		0x0d, 0x0a			; CR LF
	DB		0
chclr:
	DW		0x000f

bootpack: