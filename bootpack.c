#include "asmfunc.h"
#include "desctable.h"
#include "device.h"
#include "fifo.h"
#include "general.h"
#include "graphic.h"
#include "interrupt.h"
#include "memory.h"
#include "mtask.h"
#include "mylibgcc.h"
#include "sheet.h"
#include "timer.h"
#include "window.h"

void task_b_main(struct SHEET* sht_back);

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
    struct TIMER* timer[2];
    int mx, my;
    uint memtotal;
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct SHTCTL* shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_win_key, *sht_win_b[3];
    unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_win_key, *buf_win_b;

    struct TASK *task_a, *task_b[3];

    init_gdtidt();
    init_pic();
    io_sti(); // remove a prohibition of interrupt since IDT/PIC initialization has finished
    fifo32_init(&fifo, FIFO_SIZ, fifobuf, NULL);
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
    task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 0);

    sht_win_key = sheet_alloc(shtctl);

    buf_win_key = (unsigned char*)memman_alloc_4k(memman, 320 * 200);

    // sht_back
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char*)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    {
        sprintf(buf, "Current progress: Day 16");
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

        sprintf(buf, "2020/09/08 07:15 JST");
        putfonts8_sht(sht_back, 70, sht_back->bysize - 21, COL8_000000, COL8_C6C6C6, buf, strlen(buf));

        sprintf(buf, "size: %d x %d", sht_back->bxsize, sht_back->bysize);
        putfonts8_sht(sht_back, sht_back->bxsize - 200, sht_back->bysize - 21, COL8_000000, COL8_C6C6C6, buf, strlen(buf));
    }

    // sht_win_b
    int task_b_priority[3] = { 1, 2, 3 };
    for (int i = 0; i < 3; ++i) {
        sht_win_b[i] = sheet_alloc(shtctl);
        buf_win_b = (unsigned char*)memman_alloc_4k(memman, 144 * 52);
        sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1);
        sprintf(buf, "task_b%d", i);
        make_window8(buf_win_b, 144, 52, buf, 0);
        task_b[i] = task_alloc();
        task_b[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
        task_b[i]->tss.eip = (int)&task_b_main;
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8;
        task_b[i]->tss.ss = task_b[i]->tss.ds = task_b[i]->tss.fs = task_b[i]->tss.gs = 1 * 8;
        *((int*)(task_b[i]->tss.esp + 4)) = (int)sht_win_b[i];
        task_run(task_b[i], 2, task_b_priority[i]);
    }

    // sht_win
    sht_win = sheet_alloc(shtctl);
    buf_win = (unsigned char*)memman_alloc_4k(memman, 160 * 68);
    sheet_setbuf(sht_win, buf_win, 160, 68, -1);
    make_window8(buf_win, 160, 68, "task_a", 1);
    //putfonts8(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
    putfonts8(buf_win, 160, 24, 28 + 16, COL8_000000, "Haribote OS.");

    // sht_win_key
    sheet_setbuf(sht_win_key, buf_win_key, 320, 200, -1);
    make_window8(buf_win_key, 320, 200, "keyboard", 0);
    // 32 x 8
#define maxline 8
#define maxchar 32
    make_textbox8(sht_win_key, 10, 25 + 32, 8 * maxchar, 16 * maxline, COL8_FFFFFF);
    sheet_refresh(sht_win_key, 0, 0, 320, 200);
    char textbox[maxline][maxchar];
    int cursor_c = COL8_FFFFFF;
    int cursor_l = 0;
    int linech[maxline] = { 0 };
    char lshift = FALSE, rshift = FALSE;

    sprintf(buf, "test text here");
    putfonts8_sht(sht_win_key, 10, 25, COL8_000000, COL8_C6C6C6, buf, strlen(buf));

    // sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);
    sheet_slide(sht_win_key, 320, 200);
    sheet_slide(sht_win_b[0], 168, 56);
    sheet_slide(sht_win_b[1], 8, 116);
    sheet_slide(sht_win_b[2], 168, 116);

    sheet_updown(sht_back, 0);
    sheet_updown(sht_win_key, 1);
    for (int i = 0; i < 3; ++i) sheet_updown(sht_win_b[i], i + 2);
    sheet_updown(sht_win, 5);
    sheet_updown(sht_mouse, 6);

    int timeitv[]
        = { 50, 2 };
    int timedat[] = { 1, 5 };
    for (int i = 0; i < 2; ++i) {
        timer[i] = timer_alloc();
        timer_init(timer[i], &fifo, timedat[i]);
        timer_settime(timer[i], timeitv[i]);
    }

    // はい、よーいスタート
    ENABLE_TIMECNT = TRUE;

    for (;;) {
        io_cli();
        if (!fifo32_status(&fifo)) {
            task_sleep(task_a);
            io_sti();
        } else {
            //sprintf(buf, "fifo:[%3d]", fifo32_status(&fifo));
            //putfonts8_sht(sht_back, 0, 97, COL8_FFFFFF, COL8_008484, buf, 10);
            int data = fifo32_get(&fifo);
            io_sti();
            if (data & KEYSIG_BIT) {
                data &= ~KEYSIG_BIT;
                sprintf(buf, "%02X %08b", data, data);
                putfonts8_sht(sht_back, 0, 49, COL8_FFFFFF, COL8_008484, buf, 11);
                char release = data & 0x80;
                data &= 0x7f;
                if (keycode_tochar(data, FALSE))
                    sprintf(buf, "[%c] %s", keycode_tochar(data, FALSE), release ? "release" : "push   ");
                else
                    sprintf(buf, "[%s] %s", keycode_toname(data), release ? "release" : "push   ");
                putfonts8_sht(sht_win_key, 10, 25, COL8_000000, COL8_C6C6C6, buf, 20);

                if (!release) {
                    char ch = keycode_tochar(data, lshift || rshift);
                    linech[cursor_l];
                    // Enter
                    if (!ch && data == 0x1c && cursor_l + 1 < maxline) {
                        putfonts8_sht(sht_win_key, 10 + linech[cursor_l] * 8, 25 + 32 + cursor_l * 16, COL8_008484, COL8_FFFFFF, " ", 1);
                        cursor_l++;
                    } else if (!ch && data == 0x0e && (cursor_l > 0 || linech[cursor_l] > 0)) {
                        putfonts8_sht(sht_win_key, 10 + linech[cursor_l] * 8, 25 + 32 + cursor_l * 16, COL8_008484, COL8_FFFFFF, " ", 1);
                        if (linech[cursor_l] > 0) {
                            textbox[cursor_l][--linech[cursor_l]] = 0;
                        } else {
                            cursor_l--;
                        }
                    } else if (ch && (cursor_l + 1 < maxline || linech[cursor_l] + 1 < maxchar)) {
                        textbox[cursor_l][linech[cursor_l]++] = ch;
                        if (linech[cursor_l] + 1 == maxchar && cursor_l + 1 < maxline)
                            cursor_l++;
                    } else if (data == 0x2a)
                        lshift = TRUE;
                    else if (data == 0x36)
                        rshift = TRUE;

                    for (int l = 0; l < maxline; ++l) {
                        putfonts8_sht(sht_win_key, 10, 25 + 32 + l * 16, COL8_000000, COL8_FFFFFF, textbox[l], linech[l]);
                    }

                    putfonts8_sht(sht_win_key, 10 + linech[cursor_l] * 8, 25 + 32 + cursor_l * 16, COL8_008484, cursor_c, " ", 1);
                    sheet_refresh(sht_win_key, 0, 0, 320, 200);
                } else {
                    if (data == 0x2a)
                        lshift = FALSE;
                    else if (data == 0x36)
                        rshift = FALSE;
                }
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

                    if (mdec.btn & 0x01) {
                        sheet_updown(sht_win, 3);
                        sheet_slide(sht_win, mx - 80, my - 8);
                    } else
                        sheet_updown(sht_win, 1);
                }
            } else if (data == 5) {
                sprintf(buf, "%10d", timerctl.count);
                putfonts8_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, buf, 10);
                timer_settime(timer[1], 2);
            } else {
                switch (data) {
                /*
                case 10:
                    putfonts8_sht(sht_back, 0, 128, COL8_FFFFFF, COL8_008484, "10", 2);
                    break;
                case 3:
                    putfonts8_sht(sht_back, 8 * 2, 128, COL8_FFFFFF, COL8_008484, " 3", 2);
                    break;
                */
                case 0:
                case 1:
                    timer_init(timer[0], NULL, !data);
                    timer_settime(timer[0], timeitv[0]);
                    cursor_c = data ? COL8_000000 : COL8_FFFFFF;
                    putfonts8_sht(sht_win_key, 10 + linech[cursor_l] * 8, 25 + 32 + cursor_l * 16, COL8_008484, cursor_c, " ", 1);

                    sprintf(buf, "%d / %d", timerctl.next, timerctl.count);
                    putfonts8_sht(sht_back, 70, binfo->scrny - 45, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
                    break;
                }
            }
            //sprintf(buf, "fifo:[%3d]", fifo32_status(&fifo));
            //putfonts8_sht(sht_back, 0, 97, COL8_FFFFFF, COL8_008484, buf, 10);
        }
    }
    return;
}

uint rand_xor(void) {
    static uint y = 2463534242;
    y = y ^ (y << 13);
    y = y ^ (y >> 17);
    return y = y ^ (y << 5);
}

void task_b_main(struct SHEET* sht_win_b) {
    struct FIFO32 fifo;
    struct TIMER *timer_1sec;
    int fifobuf[128];
    char s[128];

    fifo32_init(&fifo, 128, fifobuf, NULL);
    timer_1sec = timer_alloc();
    timer_init(timer_1sec, &fifo, 100);
    timer_settime(timer_1sec, 100);

    for (int cnt = 0, cnt0 = 0;; ++cnt) {
        io_cli();
        if (!fifo32_status(&fifo))
            io_sti();
        else {
            int i = fifo32_get(&fifo);
            io_sti();
            if (i == 100) {
                sprintf(s, "%11u", cnt - cnt0);
                putfonts8_sht(sht_win_b, 24, 28, COL8_000000, COL8_C6C6C6, s, strlen(s));
                cnt0 = cnt;
                timer_settime(timer_1sec, 100);
            }
        }
    }
}
