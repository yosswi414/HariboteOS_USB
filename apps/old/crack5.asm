[BITS 32]
        GLOBAL  HariMain
HariMain:
        CALL    2*8:0x3d42  ; io_cli
        MOV     EDX, 4
        INT     0x40
