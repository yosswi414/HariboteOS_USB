[BITS 32]
[SECTION .text]
    GLOBAL  HariMain
HariMain:
    MOV     EDX, 2          ; cons_putstr0
    MOV     EBX, msg        ; s = msg
    INT     0x40
    MOV     EDX, 4          ; exit
    INT     0x40

[SECTION .data]
msg:
    DB      "HELLO WORLD", 0x0a, 0