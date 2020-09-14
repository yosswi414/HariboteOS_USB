[BITS 32]
    MOV     ECX, msg
    MOV     EDX, 1         ; cons_putchar
putloop:
    MOV     AL, [CS:ECX]
    CMP     AL, 0
    JE      fin
    INT     0x40
    ADD     ECX, 1
    JMP     putloop
fin:
    RETF
msg:
    DB      "HELLO WORLD", 0x0a, 0