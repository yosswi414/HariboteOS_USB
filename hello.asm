[BITS 32]
    MOV     EDX, 2         ; cons_putstr0
    MOV     EBX, msg       ; s = msg
    INT     0x40
    RETF
msg:
    DB      "HELLO WORLD", 0x0a, 0