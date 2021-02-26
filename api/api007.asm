[BITS 32]
        GLOBAL  api_initmalloc

[SECTION .text]

api_initmalloc: ; void api_initmalloc();
        PUSH    EBX
        MOV     EDX, 8
        MOV     EBX, [CS:0x0020]; address of malloc area
        MOV     EAX, EBX
        ADD     EAX, 32*1024    ; + 32 KB
        MOV     ECX, [CS:0x0000]; size of data segment
        SUB     ECX, EAX
        INT     0x40
        POP     EBX
        RET
