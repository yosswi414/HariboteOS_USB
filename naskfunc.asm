; naskfunc
; TAB=4

;[FORMAT "WCOFF"]				; mode to create objct file; not available in NASM
;[INSTRSET "i486p"]				; declaration to access 486 instruction; not available in NASM
[BITS 32]						; machine lang for 32bit mode
;[FILE "naskfunc.nas"]			; source file name; not available in NASM

		GLOBAL	io_hlt, io_cli, io_sti, io_stihlt
		GLOBAL	io_in8,  io_in16,  io_in32
		GLOBAL	io_out8, io_out16, io_out32
		GLOBAL	io_load_eflags, io_store_eflags
		GLOBAL	load_gdtr, load_idtr
		GLOBAL	asm_inthandler20, asm_inthandler21, asm_inthandler27, asm_inthandler2c
		EXTERN	inthandler20, inthandler21, inthandler27, inthandler2c
		GLOBAL	load_cr0, store_cr0
		GLOBAL	memtest_sub
		GLOBAL	load_tr, farjmp
		GLOBAL	farcall
		GLOBAL	asm_cons_putchar
		EXTERN	cons_putchar
		GLOBAL	asm_hrb_api
		EXTERN	hrb_api
		GLOBAL	start_app, asm_end_app
		GLOBAL	asm_inthandler00, asm_inthandler06, asm_inthandler0c, asm_inthandler0d
		EXTERN	inthandler00, inthandler06, inthandler0c, inthandler0d

[SECTION .text]

io_hlt:	; void io_hlt(void);
		HLT
		RET

io_cli:	; void io_cli(void);
		CLI
		RET

io_sti:	; void io_sti(void);
		STI
		RET

io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

io_in8:	; int io_in8(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AL,DX
		RET

io_in16:	; int io_in16(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AX,DX
		RET

io_in32:	; int io_in32(int port);
		MOV		EDX,[ESP+4]		; port
		IN		EAX,DX
		RET

io_out8:	; void io_out8(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		AL,[ESP+8]		; data
		OUT		DX,AL
		RET

io_out16:	; void io_out16(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,AX
		RET

io_out32:	; void io_out32(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,EAX
		RET

io_load_eflags:	; int io_load_eflags(void);
		PUSHFD		; PUSH EFLAGS
		POP		EAX
		RET

io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS
		RET

load_gdtr:		; void load_gdtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LGDT	[ESP+6]
		RET

load_idtr:		; void load_idtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LIDT	[ESP+6]
		RET

asm_inthandler00:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler00
		CMP		EAX, 0
		JNE		asm_end_app
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP, 4		; INT 0x00 needs this ?
		IRETD

asm_inthandler06:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler06
		CMP		EAX, 0
		JNE		asm_end_app
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP, 4		; INT 0x06 needs this ?
		IRETD

asm_inthandler0c:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler0c
		CMP		EAX, 0
		JNE		asm_end_app
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP, 4		; INT 0x0c needs this
		IRETD

asm_inthandler0d:
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler0d
		CMP		EAX, 0
		JNE		asm_end_app
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP, 4		; INT 0x0d needs this
		IRETD

asm_inthandler20:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

asm_inthandler21:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler21
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX, ESP
		PUSH	EAX
		MOV		AX, SS
		MOV		DS, AX
		MOV		ES, AX
		CALL	inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

load_cr0:		; int load_cr0(void);
		MOV		EAX, CR0
		RET
store_cr0:		; void store_cr0(int cr0);
		MOV		EAX, [ESP+4]
		MOV		CR0, EAX
		RET

memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end);
		PUSH	EDI
		PUSH	ESI
		PUSH	EBX
		MOV		ESI, 0xaa55aa55	; pat0
		MOV		EDI, 0x55aa55aa	; pat1
		MOV		EAX, [ESP+12+4]	; start
_mts_loop:
		MOV		EBX, EAX
		ADD		EBX, 0xffc
		MOV		EDX, [EBX]
		MOV		[EBX], ESI
		XOR		DWORD [EBX], 0xffffffff
		CMP		EDI, [EBX]
		JNE		_mts_fin
		XOR		DWORD [EBX], 0xffffffff
		CMP		ESI, [EBX]
		JNE		_mts_fin
		MOV		[EBX], EDX
		ADD		EAX, 0x100000
		CMP		EAX, [ESP+12+8]	; end
		JBE		_mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
_mts_fin:
		MOV		[EBX], EDX
		POP		EBX
		POP		ESI
		POP		EDI
		RET

load_tr:		; void load_tr(int tr);
		LTR		[ESP+4]
		RET

farjmp:			; void farjmp(int eip, int cs);
		JMP		FAR [ESP+4]
		RET

farcall:		; void farcall(int eip, int cs);
		CALL	FAR	[ESP+4]
		RET

asm_cons_putchar:
		STI
		PUSHAD
		PUSH	1				; move = TRUE
		AND		EAX, 0x00ff		; AH = 0
		PUSH	EAX				; chr = EAX
		PUSH	DWORD [0x0fec]	; cons = 0x0fec
		CALL	cons_putchar
		ADD		ESP, 3*4		; pop data on stack
		POPAD
		IRETD

asm_hrb_api:
		STI
		PUSH	DS
		PUSH	ES
		PUSHAD					; to save
		PUSHAD					; to pass to hrb_api
		MOV		AX, SS
		MOV		DS, AX			; copy segment for OS to DS and ES
		MOV		ES, AX
		CALL	hrb_api
		CMP		EAX, 0
		JNE		asm_end_app
		ADD		ESP, 32
		POPAD
		POP		ES
		POP		DS
		IRETD
asm_end_app:
		; EAX = address of tss.esp0
		MOV		ESP, [EAX]
		MOV		DWORD [EAX+4], 0
		POPAD
		RET		; get back to cmd_app

start_app:		; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
		PUSHAD
		MOV		EAX, [ESP+36]	; EIP for app
		MOV		ECX, [ESP+40]	; CS for app
		MOV		EDX, [ESP+44]	; ESP for app
		MOV		EBX, [ESP+48]	; DS/SS for app
		MOV		EBP, [ESP+52]	; address of tss.esp0
		MOV		[EBP], ESP		; save ESP for OS
		MOV		[EBP+4], SS		; save SS for OS
		MOV		ES, BX
		MOV		DS, BX
		MOV		FS, BX
		MOV		GS, BX

		; adjust stack to allow RETF jump to app

		OR		ECX, 3
		OR		EBX, 3
		PUSH	EBX				; SS
		PUSH	EDX				; ESP
		PUSH	ECX				; CS
		PUSH	EAX				; EIP
		RETF
