[BITS 32]
        GLOBAL  HariMain
HariMain:
        MOV     AL, 0x34
        OUT     0x43, AL    ; io_out8(PIT_CTRL, 0x34);
        MOV     AL, 0xff
        OUT     0x40, AL    ; io_out8(PIT_CNT0, 0xff);
        MOV     AL, 0xff
        OUT     0x40, AL    ; io_out8(PIT_CNT0, 0xff);

        MOV     EDX, 4      ; api_end();
        INT     0x40
