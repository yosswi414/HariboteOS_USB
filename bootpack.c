#include "asmfunc.h"
#include "desctable.h"
#include "device.h"
#include "fifo.h"
#include "general.h"
#include "graphic.h"
#include "interrupt.h"
#include "memory.h"
#include "mylibgcc.h"
#include "sheet.h"
#include "timer.h"
#include "window.h"

int dbg_val[4];
char ENABLE_TIMECNT = FALSE;

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;

#define KEYBDBUF_SIZ 32
#define MOUSEBUF_SIZ 32

void disable_time() {
    ENABLE_TIMECNT = FALSE;
}
void enable_time() {
    ENABLE_TIMECNT = TRUE;
}

void HariMain(void) {
    struct BOOTINFO* binfo = (struct BOOTINFO*)ADDR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO8 timerfifo[3];
    char buf[128], keybuf[KEYBDBUF_SIZ], mousebuf[3 * MOUSEBUF_SIZ], timerbuf[3][8];
    struct TIMER* timer[3];
    int mx, my;
    uint memtotal;
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct SHTCTL* shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    init_gdtidt();
    init_pic();
    io_sti(); // remove a prohibition of interrupt since IDT/PIC initialization has finished
    fifo8_init(&keyfifo, KEYBDBUF_SIZ, keybuf);
    fifo8_init(&mousefifo, 3 * MOUSEBUF_SIZ, mousebuf);

    init_pit();
    io_out8(PIC0_IMR, 0xf8); // accept interrupt by IRQ0-2
    io_out8(PIC1_IMR, 0xef); // accept interrupt by IRQ12
    init_keyboard();
    enable_mouse(&mdec);

    //for (int i = 0; i < 128; ++i) buf[i] = 0;

    memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);
    buf_back = (unsigned char*)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    buf_win = (unsigned char*)memman_alloc_4k(memman, 160 * 68);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    sheet_setbuf(sht_win, buf_win, 160, 68, -1);

    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    make_window8(buf_win, 160, 68, "window");
    //putfonts8(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
    putfonts8(buf_win, 160, 24, 28 + 16, COL8_000000, "Haribote OS.");

    sheet_slide(sht_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);

    sheet_updown(sht_back, 0);
    sheet_updown(sht_win, 1);
    sheet_updown(sht_mouse, 2);

    sprintf(buf, "Current progress: Day 12");
    putfonts8(buf_back, binfo->scrnx, 1, 1, COL8_848400, buf);
    putfonts8(buf_back, binfo->scrnx, 0, 0, COL8_FFFF00, buf);
    sprintf(buf, "kadai yaba~i");
    putfonts8(buf_back, binfo->scrnx, 8 * 24 + 16 + 1, 1, COL8_840084, buf);
    putfonts8(buf_back, binfo->scrnx, 8 * 24 + 16, 0, COL8_FF00FF, buf);
    sprintf(buf, "Available memory : %d KB", memtotal >> 10);
    putfonts8(buf_back, binfo->scrnx, 0, 17, COL8_FFFFFF, buf);
    sprintf(buf, "Free memory      : %d KB", memman_total(memman) >> 10);
    putfonts8(buf_back, binfo->scrnx, 0, 33, COL8_FFFFFF, buf);

    sprintf(buf, "2020/08/21 12:32 JST");
    putfonts8(buf_back, binfo->scrnx, 70, binfo->scrny - 21, COL8_000000, buf);

    sheet_refresh(sht_back, 0, 0, binfo->scrnx, binfo->scrny);

    int timeitv[] = { 1000, 300, 50 };
    for (int i = 0; i < 3; ++i) {
        fifo8_init(&timerfifo[i], 8, timerbuf[i]);
        timer[i] = timer_alloc();
        timer_init(timer[i], &timerfifo[i], 1);
        timer_settime(timer[i], timeitv[i]);
    }
    ENABLE_TIMECNT = TRUE;

    for (;;) {
        io_cli();
        sprintf(buf, "%10d", timerctl.count);
        boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 40 + 8 * 10 - 1, 43);
        putfonts8(buf_win, 160, 40, 28, COL8_000000, buf);
        sheet_refresh(sht_win, 40, 28, 120, 44);

        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo[0]) + fifo8_status(&timerfifo[1]) + fifo8_status(&timerfifo[2])
            == 0)
            io_sti();
        else {
            if (fifo8_status(&keyfifo)) {
                int dat = fifo8_get(&keyfifo);
                io_sti();
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 49, 16 * 11 - 1, 64);
                sprintf(buf, "%02X %08b", dat, dat);
                putfonts8(buf_back, binfo->scrnx, 0, 49, COL8_FFFFFF, buf);
                sheet_refresh(sht_back, 0, 49, 16 * 11 - 1, 64);
            } else if (fifo8_status(&mousefifo)) {
                int dat = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, dat)) {
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 65, 8 * 9 - 1, 128);
                    sprintf(buf, "[lcr]");
                    if (mdec.btn & 0x01) buf[1] = 'L';
                    if (mdec.btn & 0x02) buf[3] = 'R';
                    if (mdec.btn & 0x04) buf[2] = 'C';
                    putfonts8(buf_back, binfo->scrnx, 0, 65, COL8_FFFFFF, buf);
                    sprintf(buf, "%02X %02X %02X", mdec.buf[0], mdec.buf[1], mdec.buf[2]);
                    putfonts8(buf_back, binfo->scrnx, 0, 81, COL8_FFFFFF, buf);
                    sprintf(buf, "que:[%3d]", fifo8_status(&mousefifo) / 3);
                    putfonts8(buf_back, binfo->scrnx, 0, 97, COL8_FFFFFF, buf);
                    sprintf(buf, "%s", (fifo8_status(&mousefifo) / 3 >= MOUSEBUF_SIZ) ? "BufOvflw" : "");
                    putfonts8(buf_back, binfo->scrnx, 0, 113, COL8_FFFFFF, buf);
                    sheet_refresh(sht_back, 0, 65, 8 * 9 - 1, 128);

                    // move mouse cursor
                    mx = clamp(mx + mdec.x, 0, binfo->scrnx - 1);
                    my = clamp(my + mdec.y, 0, binfo->scrny - 1);
                    sheet_slide(sht_mouse, mx, my);
                }
            } else {
                if (fifo8_status(&timerfifo[0])) {
                    int dat = fifo8_get(&timerfifo[0]);
                    putfonts8(buf_back, binfo->scrnx, 0, 128, COL8_FFFFFF, "10");
                    sheet_refresh(sht_back, 0, 128, 8 * 2, 144);
                }
                if (fifo8_status(&timerfifo[1])) {
                    int dat = fifo8_get(&timerfifo[1]);
                    putfonts8(buf_back, binfo->scrnx, 8 * 2, 128, COL8_FFFFFF, " 3");
                    sheet_refresh(sht_back, 8 * 2, 128, 8 * 4, 144);
                }
                if (fifo8_status(&timerfifo[2])) {
                    int dat = fifo8_get(&timerfifo[2]);
                    timer_init(timer[2], NULL, !dat);
                    timer_settime(timer[2], timeitv[2]);
                    boxfill8(buf_win, 160, dat ? COL8_000000 : COL8_C6C6C6, 24 + 8 * 12, 28 + 16, 24 + 8 * 13 - 1, 28 + 32 - 1);
                    sheet_refresh(sht_win, 24 + 8 * 12, 28 + 16, 24 + 8 * 13 - 1, 28 + 32 - 1);
                }
                io_sti();
            }
        }
    }
    return;
}