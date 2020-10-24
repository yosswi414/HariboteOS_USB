[BITS 32]
        GLOBAL  HariMain
HariMain:
        CLI             ; prohibits interrupt so that the cpu executes HLT forever
fin:
        HLT
        JMP     fin
