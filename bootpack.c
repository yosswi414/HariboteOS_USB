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

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;
extern char ENABLE_TIMECNT;

#define KEYBDBUF_SIZ 32
#define MOUSEBUF_SIZ 3
#define FIFO_SIZ 0x80


void HariMain(void) {
    struct BOOTINFO* binfo = (struct BOOTINFO*)ADDR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo;
    char buf[128];
    int fifobuf[FIFO_SIZ];
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
    fifo32_init(&fifo, FIFO_SIZ, fifobuf);
    init_keyboard(&fifo, KEYSIG_BIT);
    enable_mouse(&fifo, MOUSESIG_BIT, &mdec);

    init_pit();
    io_out8(PIC0_IMR, 0xf8); // accept interrupt by IRQ0-2
    io_out8(PIC1_IMR, 0xef); // accept interrupt by IRQ12

    //for (int i = 0; i < 128; ++i) buf[i] = 0;

    memtotal = memtest(0x00400000, 0xffffffff);
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

    sprintf(buf, "Current progress: Day 13");
    putfonts8(buf_back, binfo->scrnx, 1, 1, COL8_848400, buf);
    putfonts8(buf_back, binfo->scrnx, 0, 0, COL8_FFFF00, buf);
    sprintf(buf, "kadai yaba~i");
    putfonts8(buf_back, binfo->scrnx, 8 * 24 + 16 + 1, 1, COL8_840084, buf);
    putfonts8(buf_back, binfo->scrnx, 8 * 24 + 16, 0, COL8_FF00FF, buf);
    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 20);

    sprintf(buf, "Available memory : %d KB", memtotal >> 10);
    putfonts8_sht(sht_back, 0, 17, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
    sprintf(buf, "Free memory      : %d KB", memman_total(memman) >> 10);
    putfonts8_sht(sht_back, 0, 33, COL8_FFFFFF, COL8_008484, buf, strlen(buf));

    sprintf(buf, "2020/08/22 14:27 JST");
    putfonts8_sht(sht_back, 70, binfo->scrny - 21, COL8_000000, COL8_C6C6C6, buf, strlen(buf));

    int timeitv[] = { 1000, 300, 50 };
    int timedat[] = { 10, 3, 1 };
    for (int i = 0; i < 3; ++i) {
        timer[i] = timer_alloc();
        timer_init(timer[i], &fifo, timedat[i]);
        timer_settime(timer[i], timeitv[i]);
    }

    ENABLE_TIMECNT = TRUE;

    for (uint cnt = 0;; ++cnt) {
        io_cli();
        //sprintf(buf, "%10d", timerctl.count);
        //putfonts8_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, buf, 10);
        if (!fifo32_status(&fifo))
            io_sti();
        else {
            sprintf(buf, "fifo:[%3d]", fifo32_status(&fifo));
            putfonts8_sht(sht_back, 0, 97, COL8_FFFFFF, COL8_008484, buf, 10);
            int data = fifo32_get(&fifo);
            io_sti();
            if (data & KEYSIG_BIT) {
                data &= ~KEYSIG_BIT;
                sprintf(buf, "%02X %08b", data, data);
                putfonts8_sht(sht_back, 0, 49, COL8_FFFFFF, COL8_008484, buf, 11);
            } else if (data & MOUSESIG_BIT) {
                data &= ~MOUSESIG_BIT;
                if (mouse_decode(&mdec, data)) {
                    sprintf(buf, "[lcr]");
                    if (mdec.btn & 0x01) buf[1] = 'L';
                    if (mdec.btn & 0x02) buf[3] = 'R';
                    if (mdec.btn & 0x04) buf[2] = 'C';
                    putfonts8_sht(sht_back, 0, 65, COL8_FFFFFF, COL8_008484, buf, 5);
                    sprintf(buf, "%02X %02X %02X", mdec.buf[0], mdec.buf[1], mdec.buf[2]);
                    putfonts8_sht(sht_back, 0, 81, COL8_FFFFFF, COL8_008484, buf, 8);
                    sprintf(buf, "%s", (fifo32_status(&fifo) >= FIFO_SIZ) ? "BufOvflw" : "        ");
                    putfonts8_sht(sht_back, 0, 113, COL8_FFFFFF, COL8_008484, buf, 8);

                    // move mouse cursor
                    if (mdec.x || mdec.y) {
                        mx = clamp(mx + mdec.x, 0, binfo->scrnx - 1);
                        my = clamp(my + mdec.y, 0, binfo->scrny - 1);
                        sheet_slide(sht_mouse, mx, my);
                    }
                }
            } else {
                switch (data) {
                case 10:
                    putfonts8_sht(sht_back, 0, 128, COL8_FFFFFF, COL8_008484, "10", 2);
                    break;
                case 3:
                    putfonts8_sht(sht_back, 8 * 2, 128, COL8_FFFFFF, COL8_008484, " 3", 2);
                    break;
                case 0:
                case 1:
                    timer_init(timer[2], NULL, !data);
                    timer_settime(timer[2], timeitv[2]);
                    putfonts8_sht(sht_win, 24 + 8 * 12, 28 + 16, COL8_FFFFFF, data ? COL8_000000 : COL8_C6C6C6, " ", 1);
                    sprintf(buf, "%10d", cnt);
                    putfonts8_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, buf, 10);

                    sprintf(buf, "%d / %d", timerctl.next, timerctl.count);
                    putfonts8_sht(sht_back, 70, binfo->scrny - 21-22, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
                    cnt = 0;
                    break;
                }
            }
            sprintf(buf, "fifo:[%3d]", fifo32_status(&fifo));
            putfonts8_sht(sht_back, 0, 97, COL8_FFFFFF, COL8_008484, buf, 10);
        }
    }
    return;
}