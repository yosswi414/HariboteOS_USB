[BITS 32]
        GLOBAL  api_putchar, api_end

[SECTION .text]

api_putchar:    ; void api_putchar(int c);
        MOV     EDX, 1
        MOV     AL, [ESP+4] ; c = *(ESP+4)
        INT     0x40
        RET

api_end:        ; void api_end(vodi);
        MOV     EDX, 4
        INT     0x40