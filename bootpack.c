#include "asmfunc.h"
#include "desctable.h"
#include "device.h"
#include "fifo.h"
#include "graphic.h"
#include "interrupt.h"
#include "mylibgcc.h"

int dbg_val[4];

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

#define MOUSEBUF_SIZ 32

void HariMain(void) {
    struct BOOTINFO* binfo = (struct BOOTINFO*)ADDR_BOOTINFO;
    struct MOUSE_DEC mdec;
    char mcursor[256], buf[128], keybuf[32], mousebuf[3 * MOUSEBUF_SIZ];
    int mx, my;

    init_gdtidt();
    init_pic();
    io_sti(); // remove a prohibition of interrupt since IDT/PIC initialization has finished
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 3 * 32, mousebuf);
    io_out8(PIC0_IMR, 0xf9); // accept interrupt by IRQ1, IRQ2
    io_out8(PIC1_IMR, 0xef); // accept interrupt by IRQ12
    init_keyboard();
    for (int i = 0; i < 128; ++i) buf[i] = 0;

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    enable_mouse(&mdec);

    sprintf(buf, "Current progress: Day 09");
    putfonts8(binfo->vram, binfo->scrnx, 3, 1, COL8_848400, buf);
    putfonts8(binfo->vram, binfo->scrnx, 2, 0, COL8_FFFF00, buf);

    sprintf(buf, "2020/08/14 15:38 JST");
    putfonts8(binfo->vram, binfo->scrnx, 70, binfo->scrny - 21, COL8_000000, buf);

    for (;;) {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
            io_stihlt();
        else {
            if (fifo8_status(&keyfifo)) {
                int dat = fifo8_get(&keyfifo);
                io_sti();
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 100, 47);
                sprintf(buf, "%02X", dat);
                putfonts8(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, buf);
                sprintf(buf, "%08b", dat);
                putfonts8(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, buf);
            } else if (fifo8_status(&mousefifo)) {
                int dat = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, dat)) {
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 48, 180, 111);

                    sprintf(buf, "[lcr]");
                    if (mdec.btn & 0x01) buf[1] = 'L';
                    if (mdec.btn & 0x02) buf[3] = 'R';
                    if (mdec.btn & 0x04) buf[2] = 'C';
                    putfonts8(binfo->vram, binfo->scrnx, 32, 48, COL8_FFFFFF, buf);
                    sprintf(buf, "%02X %02X %02X", mdec.buf[0], mdec.buf[1], mdec.buf[2]);
                    putfonts8(binfo->vram, binfo->scrnx, 32, 64, COL8_FFFFFF, buf);
                    sprintf(buf, "que:[%3d]", (mousefifo.size - mousefifo.free) / 3);
                    putfonts8(binfo->vram, binfo->scrnx, 32, 80, COL8_FFFFFF, buf);
                    sprintf(buf, "%s", (mousefifo.size - mousefifo.free) / 3 >= MOUSEBUF_SIZ ? "Buffer Overflow!" : "");
                    putfonts8(binfo->vram, binfo->scrnx, 32, 96, COL8_FFFFFF, buf);

                    // move mouse cursor

                    // delete existing cursor on screen
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
                    mx += mdec.x;
                    my += mdec.y;
                    mx = max(mx, 0);
                    my = max(my, 0);
                    mx = min(mx, binfo->scrnx - 16);
                    my = min(my, binfo->scrny - 16);
                    sprintf(buf, "(%3d, %3d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
                    putfonts8(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, buf);
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
                }
            }
        }
    }

    while (1) io_hlt();
    return;
}