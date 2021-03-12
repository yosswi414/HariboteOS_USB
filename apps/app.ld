OUTPUT_FORMAT(binary);
OUTPUT_ARCH(i386)

SECTIONS
{
    .head 0x0 : {
        LONG(512 * 1024)  /*  0 : stack+.data+heap の大きさ（4KBの倍数） */
        LONG(0x69726148)      /*  4 : シグネチャ "Hari" */
        LONG(0)               /*  8 : mmarea の大きさ（4KBの倍数） */
        LONG(128 * 1024)       /* 12 : スタック初期値 (スタックサイズ) ＆.data転送先 */
        LONG(SIZEOF(.data))   /* 16 : .dataサイズ */
        LONG(LOADADDR(.data)) /* 20 : .dataの初期値列のファイル位置 */
        LONG(0xE9000000)      /* 24 : 0xE9000000 */
        LONG(HariMain - 0x20) /* 28 : エントリアドレス - 0x20 */
        LONG(256 * 1024)       /* 32 : heap領域（malloc領域）開始アドレス */
    }

    .text : {
        *(.text)
        KEEP(*(.note.*))
        *(.text.unlikely)
    }

    .data (128 * 1024 /*スタックサイズ*/) : AT ( ADDR(.text) + SIZEOF(.text) ) {
        *(.data)
        *(.rodata*)
        *(.bss)
    }

    /DISCARD/ : { *(.eh_frame) }

}
