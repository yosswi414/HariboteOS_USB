#ifndef PROGRESS_CHAPTER
#define PROGRESS_CHAPTER -1
#endif
#ifndef PROGRESS_PAGE
#define PROGRESS_PAGE -1
#endif
#ifndef PROGRESS_YEAR
#define PROGRESS_YEAR -1
#endif
#ifndef PROGRESS_MONTH
#define PROGRESS_MONTH -1
#endif
#ifndef PROGRESS_DAY
#define PROGRESS_DAY -1
#endif
#ifndef PROGRESS_HOUR
#define PROGRESS_HOUR -1
#endif
#ifndef PROGRESS_MIN
#define PROGRESS_MIN -1
#endif

#include "acpi.h"
#include "asmfunc.h"
#include "console.h"
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
#include "sysfunc.h"
#include "timer.h"
#include "window.h"

void task_b_main(struct SHEET* sht_back);

int dbg_val[4];
char dbg_str[4][64];
char acpiTrial = FALSE;
char hasAcpiTried = FALSE;
char isAcpiAvail = FALSE;
int prevAcpiTrial = 0;

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;
extern char ENABLE_TIMECNT;

#define KEYBDBUF_SIZ 32
#define MOUSEBUF_SIZ 3
#define FIFO_SIZ 0x80

#define KEYCMD_LED 0xed

#define CONS_NUM 2

void HariMain(void) {
    struct BOOTINFO* binfo = (struct BOOTINFO*)ADDR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo, keycmd;
    char buf[128];
    int fifobuf[FIFO_SIZ], keycmd_buf[32], *cons_fifo[CONS_NUM];
    struct TIMER* timer[1];
    int mx, my;
    int x, y;
    int mmx = -1, mmy = -1;
    int mhx = -1, mhy = -1;
    char hold_bar = FALSE;
    uint memtotal;
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct SHTCTL* shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_cons[CONS_NUM], *sht = NULL;
    unsigned char *buf_back, buf_mouse[256], *buf_cons[CONS_NUM];

    struct TASK *task_a, *task_cons[CONS_NUM], *task;

    struct CONSOLE* cons;

    int key_to = 0, key_shift = 0, key_ctrl = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
    struct SHEET* key_win;

    init_gdtidt();
    init_pic();
    io_sti();  // remove a prohibition of interrupt since IDT/PIC initialization has finished
    fifo32_init(&fifo, sizeof(fifobuf) / sizeof(int), fifobuf, NULL);
    init_keyboard(&fifo, KEYSIG_BIT);
    enable_mouse(&fifo, MOUSESIG_BIT, &mdec);

    init_pit();
    io_out8(PIC0_IMR, 0xf8);  // accept interrupt by IRQ0-2
    io_out8(PIC1_IMR, 0xef);  // accept interrupt by IRQ12

    fifo32_init(&keycmd, sizeof(keycmd_buf) / sizeof(int), keycmd_buf, NULL);

    //for (int i = 0; i < 128; ++i) buf[i] = 0;

    memtotal = memtest(0x00400000, 0xffffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    task_a = task_init(memman);
    fifo.task = task_a;
    task_run(task_a, 1, 0);  // level: max

    // sht_back
    sht_back = sheet_alloc(shtctl);
    *((int*)ADDR_SHTCTL) = (int)shtctl;
    buf_back = (unsigned char*)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    {
        sprintf(buf, "Current progress: Day %d, p.%d", PROGRESS_CHAPTER, PROGRESS_PAGE);
        putfonts8(buf_back, binfo->scrnx, 1, 1, COL8_848400, buf);
        putfonts8(buf_back, binfo->scrnx, 0, 0, COL8_FFFF00, buf);
        sprintf(buf, "kadai yaba~i");
        putfonts8(buf_back, binfo->scrnx, 8 * 31 + 16 + 1, 1, COL8_840084, buf);
        putfonts8(buf_back, binfo->scrnx, 8 * 31 + 16, 0, COL8_FF00FF, buf);
        sheet_refresh(sht_back, 0, 0, binfo->scrnx, 20);

        /*
        sprintf(buf, "Available memory : %d KB", memtotal >> 10);
        putfonts8_sht(sht_back, 0, 17, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
        sprintf(buf, "Free memory      : %d KB", memman_total(memman) >> 10);
        putfonts8_sht(sht_back, 0, 33, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
        */

        sprintf(buf, "%04s/%02s/%02s %02s:%02s JST", PROGRESS_YEAR, PROGRESS_MONTH, PROGRESS_DAY, PROGRESS_HOUR, PROGRESS_MIN);
        putfonts8_sht(sht_back, 70, sht_back->bysize - 21, COL8_000000, COL8_C6C6C6, buf, strlen(buf));

        sprintf(buf, "size: %d x %d", sht_back->bxsize, sht_back->bysize);
        putfonts8_sht(sht_back, sht_back->bxsize - 200, sht_back->bysize - 21, COL8_000000, COL8_C6C6C6, buf, strlen(buf));
    }

    // sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;

    // sht_cons
    int cons_height = 480;
    int cons_width = 640;
    for (int i = 0; i < CONS_NUM; ++i) {
        sht_cons[i] = sheet_alloc(shtctl);
        buf_cons[i] = (unsigned char*)memman_alloc_4k(memman, cons_width * cons_height);
        sheet_setbuf(sht_cons[i], buf_cons[i], cons_width, cons_height, -1);
        sprintf(buf, "console %d", i);
        make_window8(buf_cons[i], cons_width, cons_height, buf, 0);
        make_textbox8(sht_cons[i], 8, 28, cons_width - 16, cons_height - 37, COL8_000000);
        task_cons[i] = task_alloc();
        task_cons[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
        task_cons[i]->tss.eip = (int)&console_task;
        task_cons[i]->tss.es = 1 * 8;
        task_cons[i]->tss.cs = 2 * 8;
        task_cons[i]->tss.ss = task_cons[i]->tss.ds = task_cons[i]->tss.fs = task_cons[i]->tss.gs = 1 * 8;
        *((int*)(task_cons[i]->tss.esp + 4)) = (int)sht_cons[i];
        *((int*)(task_cons[i]->tss.esp + 8)) = memtotal;
        task_run(task_cons[i], 2, 2);  // level = priority = 2

        sht_cons[i]->task = task_cons[i];
        sht_cons[i]->flags |= SHEET_FLAGS_CURSOR;

        cons_fifo[i] = (int*)memman_alloc_4k(memman, 128 * 4);
        fifo32_init(&task_cons[i]->fifo, 128, cons_fifo[i], task_cons[i]);
    }

    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_cons[0], 356, 6);
    sheet_slide(sht_cons[1], 308, 2);
    sheet_slide(sht_mouse, mx, my);

    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons[0], 1);
    sheet_updown(sht_cons[1], 2);
    sheet_updown(sht_mouse, 3);

    key_win = sht_cons[0];
    keywin_on(key_win);

    int timeitv[] = {50};
    int timedat[] = {1};
    for (int i = 0; i < 1; ++i) {
        timer[i] = timer_alloc();
        timer_init(timer[i], &fifo, timedat[i]);
        timer_settime(timer[i], timeitv[i]);
    }

    // はい、よーいスタート
    ENABLE_TIMECNT = TRUE;

    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    for (;;) {
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }
        io_cli();
        if (timerctl.next > 50 && !hasAcpiTried && acpiTrial) {
            if (!initAcpi())
                hasAcpiTried = 1;
            else
                hasAcpiTried = -1;
        }
        if (timerctl.count > prevAcpiTrial + 400 && hasAcpiTried == 1 && acpiTrial) {
            if (!acpiEnable())
                hasAcpiTried = 2, isAcpiAvail = TRUE;
            else
                prevAcpiTrial = timerctl.count;
            //sleep(1000);
            //cons_putstr0(cons, "slept 1 sec.\n");
            // hasAcpiTried = 2;
        }
        if (!fifo32_status(&fifo)) {
            task_sleep(task_a);
            io_sti();
        } else {
            int data = fifo32_get(&fifo);
            sprintf(buf, "fifo:[%3d](%4d)", fifo32_status(&fifo), data);
            putfonts8_sht(sht_back, 0, 17, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
            io_sti();
            if (!key_win->flags) {
                key_win = shtctl->sheets[shtctl->top - 1];
                keywin_on(key_win);
            }
            if (data & KEYSIG_BIT) {  // KEYBOARD
                data &= ~KEYSIG_BIT;
                sprintf(buf, "%02X %08b", data, data);
                putfonts8_sht(sht_back, 0, 49, COL8_FFFFFF, COL8_008484, buf, 11);

                if (data == 0xfa)  // led info received
                    keycmd_wait = -1;
                if (data == 0xfe) {  // led info not reached
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }

                char release = data & 0x80;
                data &= 0x7f;
                if (keycode_tochar(data, FALSE))
                    sprintf(buf, "[%c] %s", keycode_tochar(data, FALSE), release ? "release" : "push   ");
                else
                    sprintf(buf, "[%s] %s", keycode_toname(data), release ? "release" : "push   ");

                putfonts8_sht(sht_back, 0, 129, COL8_FFFFFF, COL8_008484, buf, 20);

                if (!release) {
                    task = key_win->task;
                    char ch = keycode_tochar(data, !!key_shift ^ !!(key_leds & 4));
                    if (data == 0x1c) {  // Enter
                        fifo32_put(&task->fifo, KEYSIG_BIT | '\n');
                    }
                    if (data == 0x0e) {  // Backspace
                        fifo32_put(&task->fifo, KEYSIG_BIT | '\b');
                    }
                    if (ch) {
                        if (key_ctrl && key_shift && tolower(ch) == 'q') {
                            cons_putstr0(task->cons, "Ctrl + Shift + Q\n");
                            goto QUIT_MAIN;
                        }
                        if (key_ctrl && tolower(ch) == 'c') {  // Ctrl + c
                            if (task && task->tss.ss0) {
                                cons_putstr0(task->cons, "User Interrupt Detected.\n");
                                cons_putstr0(task->cons, "The current process will be terminated...\n");
                                fifo32_put(&task->fifo, 127);  // sends 127 to break for loop immediately
                                io_cli();
                                task->tss.eax = (int)&(task->tss.esp0);
                                task->tss.eip = (int)asm_end_app;
                                io_sti();
                            } else {
                                cons_putchar(task->cons, ' ', FALSE);
                                cons_putstr0(task->cons, "\n>");
                            }
                        } else {
                            fifo32_put(&task->fifo, ch | KEYSIG_BIT);
                        }
                    }
                    if (data == 0x2a) {  // LShift
                        key_shift |= 1;
                    }
                    if (data == 0x36) {  // RShift
                        key_shift |= 2;
                    }
                    if (data == 0x1d) {  // LCtrl
                        key_ctrl |= 1;
                    }
                    //if (data == 0x) { // RCtrl
                    //    key_ctrl |= 2;
                    //}
                    if (data == 0x57) {  // F11
                        sheet_updown(shtctl->sheets[1], shtctl->top - 1);
                    }
                    if (data == 0x0f) {  // Tab
                        keywin_off(key_win);
                        int i = key_win->height - 1;
                        if (!i) i = shtctl->top - 1;
                        key_win = shtctl->sheets[i];
                        keywin_on(key_win);
                    }
                    if (data == 0x3a) {  // CapsLock
                        key_leds ^= 4;
                        fifo32_put(&keycmd, KEYCMD_LED);
                        fifo32_put(&keycmd, key_leds);
                    }
                    if (data == 0x45) {  // NumLock
                        key_leds ^= 2;
                        fifo32_put(&keycmd, KEYCMD_LED);
                        fifo32_put(&keycmd, key_leds);
                    }
                    if (data == 0x46) {  // ScrollLock
                        key_leds ^= 1;
                        fifo32_put(&keycmd, KEYCMD_LED);
                        fifo32_put(&keycmd, key_leds);
                    }
                } else {
                    if (data == 0x2a) {
                        key_shift &= ~1;
                    }
                    if (data == 0x36) {
                        key_shift &= ~2;
                    }
                    if (data == 0x1d) {
                        key_ctrl &= ~1;
                    }
                }
            } else if (data & MOUSESIG_BIT) {  // MOUSE
                data &= ~MOUSESIG_BIT;
                if (mouse_decode(&mdec, data)) {
                    sprintf(buf, "[lcr]{cns}");
                    if (mdec.btn & 0x01) buf[1] = 'L';
                    if (mdec.btn & 0x02) buf[3] = 'R';
                    if (mdec.btn & 0x04) buf[2] = 'C';
                    if (key_leds & 1) buf[8] = 'S';
                    if (key_leds & 2) buf[7] = 'N';
                    if (key_leds & 4) buf[6] = 'C';
                    putfonts8_sht(sht_back, 0, 65, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
                    sprintf(buf, "%02X %02X %02X", mdec.buf[0], mdec.buf[1], mdec.buf[2]);
                    putfonts8_sht(sht_back, 0, 81, COL8_FFFFFF, COL8_008484, buf, 8);
                    sprintf(buf, "%s", (fifo32_status(&fifo) >= FIFO_SIZ) ? "BufOvflw " : "Available");
                    putfonts8_sht(sht_back, 0, 113, COL8_FFFFFF, COL8_008484, buf, strlen(buf));

                    // move mouse cursor
                    if (mdec.x || mdec.y) {
                        mx = clamp(mx + mdec.x, 0, binfo->scrnx - 1);
                        my = clamp(my + mdec.y, 0, binfo->scrny - 1);
                        sheet_slide(sht_mouse, mx, my);
                    }

                    // left button push
                    if (mdec.btn & 0x01) {
                        if (mhx < 0) {
                            mhx = mmx = mx, mhy = mmy = my;
                            for (int i = shtctl->top - 1; i > 0; --i) {
                                sht = shtctl->sheets[i];
                                x = mx - sht->vx0;
                                y = my - sht->vy0;
                                if (x == clamp(x, 0, sht->bxsize - 1) && y == clamp(y, 0, sht->bysize - 1)) {
                                    if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                        sheet_updown(sht, shtctl->top - 1);
                                        if (sht != key_win) {
                                            keywin_off(key_win);
                                            key_win = sht;
                                            keywin_on(key_win);
                                        }
                                        if (sht->bxsize - x == clamp(sht->bxsize - x, 6, 21) && y == clamp(y, 5, 18))  // if close button
                                            hold_bar = FALSE;
                                        else if (x == clamp(x, 3, sht->bxsize - 4) && y == clamp(y, 3, 20))  // if title bar
                                            hold_bar = TRUE;
                                        break;
                                    }
                                }
                            }
                        } else {
                            if (hold_bar) {
                                int dx = mx - mmx;
                                int dy = my - mmy;
                                sheet_slide(sht, sht->vx0 + dx, sht->vy0 + dy);
                            }
                            mmx = mx;
                            mmy = my;
                        }

                    } else {
                        // left button release
                        if (mhx >= 0) {
                            // close button
                            x = mx - sht->vx0;
                            y = my - sht->vy0;
                            if (sht->bxsize - x == clamp(sht->bxsize - x, 6, 21) && y == clamp(y, 5, 18)) {
                                task = sht->task;
                                cons_putstr0(task->cons, "User Interrupt (Mouse) Detected.\n");
                                if (sht->flags & SHEET_FLAGS_APP) {
                                    cons_putstr0(task->cons, "The current process will be terminated...\n");
                                    fifo32_put(&task->fifo, 127);  // sends 127 to break for loop immediately
                                    io_cli();
                                    task->tss.eax = (int)&(task->tss.esp0);
                                    task->tss.eip = (int)asm_end_app;
                                    io_sti();
                                } else {
                                    cons_putstr0(task->cons, "The current process is not an app.\n");
                                }
                            }
                            mmx = mhx = -1, hold_bar = FALSE;
                            sht = NULL;
                        }
                    }
                }
            } else {
                switch (data) {
                    case 0:
                    case 1:
                        timer_init(timer[0], NULL, !data);
                        timer_settime(timer[0], timeitv[0]);

                        sprintf(buf, "%d / %d", timerctl.next, timerctl.count);
                        putfonts8_sht(sht_back, 70, binfo->scrny - 45, COL8_FFFFFF, COL8_008484, buf, strlen(buf));

                        break;
                    case 127:
                        if (key_win->task->cons) cons_putstr0(key_win->task->cons, "SIGNAL 127\n");
                        break;
                }
            }
        }
    }
QUIT_MAIN:
    return;
}

void task_b_main(struct SHEET* sht_win_b) {
    struct FIFO32 fifo;
    struct TIMER* timer_1sec;
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
