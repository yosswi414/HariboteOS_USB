[BITS 32]
    MOV     EDX, 2          ; cons_putstr0
    MOV     EBX, msg        ; s = msg
    INT     0x40
    MOV     EDX, 4          ; exit
    INT     0x40
msg:
    DB      "HELLO WORLD", 0x0a, 0