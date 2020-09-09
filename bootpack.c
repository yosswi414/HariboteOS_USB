
#define PROGRESS_CHAPTER 18
#define PROGRESS_PAGE 371
#define PROGRESS_YEAR 2020
#define PROGRESS_MONTH 9
#define PROGRESS_DAY 9
#define PROGRESS_HOUR 13
#define PROGRESS_MIN 10

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
void console_task(struct SHEET* sheet, int memtotal);
int cons_newline(int cursor_y, struct SHEET* sheet);
int dbg_val[4];

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;
extern char ENABLE_TIMECNT;

#define KEYBDBUF_SIZ 32
#define MOUSEBUF_SIZ 3
#define FIFO_SIZ 0x80

#define KEYCMD_LED 0xed

void HariMain(void) {
    struct BOOTINFO* binfo = (struct BOOTINFO*)ADDR_BOOTINFO;
    struct MOUSE_DEC mdec;
    struct FIFO32 fifo, keycmd;
    char buf[128];
    int fifobuf[FIFO_SIZ], keycmd_buf[32];
    struct TIMER* timer[1];
    int mx, my;
    uint memtotal;
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct SHTCTL* shtctl;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    struct TASK* task_a;

    int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

    init_gdtidt();
    init_pic();
    io_sti(); // remove a prohibition of interrupt since IDT/PIC initialization has finished
    fifo32_init(&fifo, sizeof(fifobuf) / sizeof(int), fifobuf, NULL);
    init_keyboard(&fifo, KEYSIG_BIT);
    enable_mouse(&fifo, MOUSESIG_BIT, &mdec);

    init_pit();
    io_out8(PIC0_IMR, 0xf8); // accept interrupt by IRQ0-2
    io_out8(PIC1_IMR, 0xef); // accept interrupt by IRQ12

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
    task_run(task_a, 1, 0); // level: max

    // sht_back
    sht_back = sheet_alloc(shtctl);
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

        sprintf(buf, "Available memory : %d KB", memtotal >> 10);
        putfonts8_sht(sht_back, 0, 17, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
        sprintf(buf, "Free memory      : %d KB", memman_total(memman) >> 10);
        putfonts8_sht(sht_back, 0, 33, COL8_FFFFFF, COL8_008484, buf, strlen(buf));

        sprintf(buf, "%04d/%02d/%02d %02d:%02d JST", PROGRESS_YEAR, PROGRESS_MONTH, PROGRESS_DAY, PROGRESS_HOUR, PROGRESS_MIN);
        putfonts8_sht(sht_back, 70, sht_back->bysize - 21, COL8_000000, COL8_C6C6C6, buf, strlen(buf));

        sprintf(buf, "size: %d x %d", sht_back->bxsize, sht_back->bysize);
        putfonts8_sht(sht_back, sht_back->bxsize - 200, sht_back->bysize - 21, COL8_000000, COL8_C6C6C6, buf, strlen(buf));
    }

    // sht_win
    int win_height = 200;
    int win_width = 320;
    sht_win = sheet_alloc(shtctl);
    buf_win = (unsigned char*)memman_alloc_4k(memman, win_width * win_height);
    sheet_setbuf(sht_win, buf_win, win_width, win_height, -1);
    make_window8(buf_win, win_width, win_height, "task_a", 1);
    // 32 x 8
#define maxline 8
#define maxchar 32
    make_textbox8(sht_win, 10, 25 + 32, 8 * maxchar, 16 * maxline, COL8_FFFFFF);
    sheet_refresh(sht_win, 0, 0, win_width, win_height);
    char textbox[maxline][maxchar];
    int cursor_col = COL8_FFFFFF;
    int cursor_line = 0;
    int linech[maxline] = { 0 };
    sprintf(buf, "text here");
    putfonts8_sht(sht_win, 10, 25, COL8_000000, COL8_C6C6C6, buf, strlen(buf));

    // sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;

    // sht_cons
    int cons_height = 480;
    int cons_width = 640;
    struct SHEET* sht_cons;
    unsigned char* buf_cons;
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char*)memman_alloc_4k(memman, cons_width * cons_height);
    sheet_setbuf(sht_cons, buf_cons, cons_width, cons_height, -1);
    make_window8(buf_cons, cons_width, cons_height, "console", 0);
    make_textbox8(sht_cons, 8, 28, cons_width - 16, cons_height - 37, COL8_000000);
    struct TASK* task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
    task_cons->tss.eip = (int)&console_task;
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = task_cons->tss.ds = task_cons->tss.fs = task_cons->tss.gs = 1 * 8;
    *((int*)(task_cons->tss.esp + 4)) = (int)sht_cons;
    *((int*)(task_cons->tss.esp + 8)) = memtotal;
    task_run(task_cons, 2, 2); // level = priority = 2

    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_cons, 350, 100);
    sheet_slide(sht_win, 80, 72);
    sheet_slide(sht_mouse, mx, my);

    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons, 1);
    sheet_updown(sht_win, 2);
    sheet_updown(sht_mouse, 3);

    int timeitv[] = { 50 };
    int timedat[] = { 1 };
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
        if (!fifo32_status(&fifo)) {
            task_sleep(task_a);
            io_sti();
        } else {
            sprintf(buf, "fifo:[%3d]", fifo32_status(&fifo));
            putfonts8_sht(sht_back, 0, 97, COL8_FFFFFF, COL8_008484, buf, strlen(buf));
            int data = fifo32_get(&fifo);
            io_sti();
            if (data & KEYSIG_BIT) { // KEYBOARD
                data &= ~KEYSIG_BIT;
                sprintf(buf, "%02X %08b", data, data);
                putfonts8_sht(sht_back, 0, 49, COL8_FFFFFF, COL8_008484, buf, 11);

                if (data == 0xfa) // led info received
                    keycmd_wait = -1;
                if (data == 0xfe) { // led info not reached
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
                    char ch = keycode_tochar(data, !!key_shift ^ !!(key_leds & 4));
                    linech[cursor_line];
                    if (data == 0x1c) { // Enter
                        if (key_to)
                            fifo32_put(&task_cons->fifo, KEYSIG_BIT | '\n');
                        else {
                            if (!ch && cursor_line + 1 < maxline) {
                                putfonts8_sht(sht_win, 10 + linech[cursor_line] * 8, 25 + 32 + cursor_line * 16, COL8_008484, COL8_FFFFFF, " ", 1);
                                cursor_line++;
                            }
                        }
                    }
                    if (data == 0x0e) { // Backspace
                        if (key_to)
                            fifo32_put(&task_cons->fifo, KEYSIG_BIT | '\b');
                        else {
                            if (!ch && (cursor_line > 0 || linech[cursor_line] > 0)) {
                                putfonts8_sht(sht_win, 10 + linech[cursor_line] * 8, 25 + 32 + cursor_line * 16, COL8_008484, COL8_FFFFFF, " ", 1);
                                if (linech[cursor_line] > 0) {
                                    textbox[cursor_line][--linech[cursor_line]] = 0;
                                } else {
                                    cursor_line--;
                                }
                            }
                        }
                    }
                    if (ch) {
                        if (key_to)
                            fifo32_put(&task_cons->fifo, ch | KEYSIG_BIT);
                        else {
                            if (cursor_line + 1 < maxline || linech[cursor_line] + 1 < maxchar) {
                                textbox[cursor_line][linech[cursor_line]++] = ch;
                                if (linech[cursor_line] + 1 == maxchar && cursor_line + 1 < maxline)
                                    cursor_line++;
                            }
                        }
                    }
                    if (data == 0x2a) { // LShift
                        key_shift |= 1;
                    }
                    if (data == 0x36) { // RShift
                        key_shift |= 2;
                    }
                    if (data == 0x0f) { // Tab
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", key_to);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", !key_to);
                        cursor_col = key_to ? COL8_000000 : -1;
                        fifo32_put(&task_cons->fifo, 2 + key_to);
                        if (!key_to)
                            putfonts8_sht(sht_win, 10 + linech[cursor_line] * 8, 25 + 32 + cursor_line * 16, COL8_008484, COL8_FFFFFF, " ", 1);

                        key_to = 1 - key_to;

                        sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
                        sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                    }
                    if (data == 0x3a) { // CapsLock
                        key_leds ^= 4;
                        fifo32_put(&keycmd, KEYCMD_LED);
                        fifo32_put(&keycmd, key_leds);
                    }
                    if (data == 0x45) { // NumLock
                        key_leds ^= 2;
                        fifo32_put(&keycmd, KEYCMD_LED);
                        fifo32_put(&keycmd, key_leds);
                    }
                    if (data == 0x46) { // ScrollLock
                        key_leds ^= 1;
                        fifo32_put(&keycmd, KEYCMD_LED);
                        fifo32_put(&keycmd, key_leds);
                    }

                    for (int l = 0; l < maxline; ++l) {
                        putfonts8_sht(sht_win, 10, 25 + 32 + l * 16, COL8_000000, COL8_FFFFFF, textbox[l], linech[l]);
                    }
                    if (cursor_col >= 0)
                        putfonts8_sht(sht_win, 10 + linech[cursor_line] * 8, 25 + 32 + cursor_line * 16, COL8_008484, cursor_col, " ", 1);
                } else {
                    if (data == 0x2a) {
                        key_shift &= ~1;
                    } else if (data == 0x36) {
                        key_shift &= ~2;
                    }
                }
            } else if (data & MOUSESIG_BIT) { // MOUSE
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

                    if (mdec.btn & 0x01) {
                        sheet_updown(sht_win, 3);
                        sheet_slide(sht_win, mx - 80, my - 8);
                    } else
                        sheet_updown(sht_win, 1);
                }
            } else {
                switch (data) {
                case 0:
                case 1:
                    timer_init(timer[0], NULL, !data);
                    timer_settime(timer[0], timeitv[0]);
                    if (cursor_col >= 0) {
                        cursor_col = data ? COL8_000000 : COL8_FFFFFF;
                        putfonts8_sht(sht_win, 10 + linech[cursor_line] * 8, 25 + 32 + cursor_line * 16, COL8_008484, cursor_col, " ", 1);
                    }

                    sprintf(buf, "%d / %d", timerctl.next, timerctl.count);
                    putfonts8_sht(sht_back, 70, binfo->scrny - 45, COL8_FFFFFF, COL8_008484, buf, strlen(buf));

                    break;
                }
            }
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

struct COORD {
    int x, y;
    int col;
};

struct FILEINFO {
    unsigned char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date, clustno;
    unsigned int size;
};

void console_task(struct SHEET* sheet, int memtotal) {
    struct TIMER* timer;
    struct TASK* task = task_now();
    char buf[128], cmdline[30];
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct FILEINFO* finfo = (struct FILEINFO*)(0x8000 + 0x2600); //(ADDR_DISKIMG + 0x002600);

    int width = (sheet->bxsize - 8 - 1) / 8 - 2, height = (sheet->bysize - 28 - 1) / 16 - 1;

    int fifobuf[128];
    fifo32_init(&task->fifo, sizeof(fifobuf) / sizeof(int), fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    struct COORD cursor;
    cursor.col = -1;
    cursor.x = 1;
    cursor.y = 0;

    putfonts8_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

    while (TRUE) {
        io_cli();
        if (!fifo32_status(&task->fifo)) {
            task_sleep(task);
            io_sti();
        } else {
            int data = fifo32_get(&task->fifo);
            io_sti();
            if (data <= 1) { // timer for cursor
                timer_init(timer, &task->fifo, 1 - data);
                if (cursor.col >= 0) cursor.col = data ? COL8_FFFFFF : COL8_000000;
                timer_settime(timer, 50);
                putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_008484, cursor.col, " ", 1);
            }
            if (data == 2) cursor.col = COL8_FFFFFF;
            if (data == 3) {
                cursor.col = -1;
                putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_008484, COL8_000000, " ", 1);
            }
            if (data & KEYSIG_BIT) {
                data &= ~KEYSIG_BIT;
                if (data == '\b') {
                    if (cursor.x > 1) {
                        putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor.x--;
                    }
                } else if (data == '\n') {
                    putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cursor.x - 1] = '\0';
                    //putfonts8_sht(sheet, 20, 20, COL8_FF0000, COL8_008484, cmdline, strlen(cmdline));
                    cursor.y = cons_newline(cursor.y, sheet);

                    if (!strcmp(cmdline, "mem")) {
                        sprintf(buf, "total %d MB", memtotal / (1 << 20));
                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.y = cons_newline(cursor.y, sheet);
                        sprintf(buf, "free  %d KB", memman_total(memman) / (1 << 10));
                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.y = cons_newline(cursor.y, sheet);
                        //cursor.y = cons_newline(cursor.y, sheet);
                    } else if (!strcmp(cmdline, "clear") || !strcmp(cmdline, "cls")) {
                        for (int y = 28; y < 28 + height * 16; ++y)
                            for (int x = 8; x < 8 + width * 8; ++x) sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                        sheet_refresh(sheet, 8, 28, 8 + width * 8, 28 + height * 16);
                        cursor.y = 0;
                    } else if (!strcmp(cmdline, "ls") || !strcmp(cmdline, "dir")) {
                        for (int x = 0; x < 224; ++x) {
                            if (finfo[x].name[0] == 0x00) {
                                sprintf(buf, "No further file found.");
                                putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                cursor.y = cons_newline(cursor.y, sheet);
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                if ((finfo[x].type & 0x18) == 0) {
                                    sprintf(buf, "filename.ext   %7d", finfo[x].size);
                                    for (int y = 0; y < 8; y++) {
                                        buf[y] = finfo[x].name[y];
                                    }
                                    buf[9] = finfo[x].ext[0];
                                    buf[10] = finfo[x].ext[1];
                                    buf[11] = finfo[x].ext[2];
                                    putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                    cursor.y = cons_newline(cursor.y, sheet);
                                }
                            }
                        }
                    } else if (!strncmp(cmdline, "dump ", 5)) {
                        //strcpy(buf, (char*)finfo);
                        //int i = 0;
                        //do {
                        //    ++i;
                        //    buf[i] = cmdline[i + 5];
                        //} while (cmdline[i]);

                        strcpy(buf, cmdline + 5);
                        int addr = atoi(buf);

                        char nullch = ',';
                        //while (*((unsigned char*)addr) == 0) addr += 0x100;
                        for (int t = 0; t < 16; ++t, addr += 0x10) {
                            buf[0] = '\"';
                            for (int k = 0; k < 16; ++k) {
                                buf[k + 1] = *((unsigned char*)addr + k);
                                if (!buf[k + 1]) buf[k + 1] = nullch;
                                //sprintf(buf, "[%c](%02x)", *((unsigned char*)addr + k), *((unsigned char*)addr + k));
                            }
                            buf[17] = '\"';
                            buf[18] = '\0';
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                        }
                        sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x100, addr, nullch);
                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.y = cons_newline(cursor.y, sheet);

                    } else if (!strncmp(cmdline, "fump ", 5)) {
                        //strcpy(buf, (char*)finfo);
                        //int i = 0;
                        //do {
                        //    ++i;
                        //    buf[i] = cmdline[i + 5];
                        //} while (cmdline[i]);

                        strcpy(buf, cmdline + 5);
                        int addr = atoi(buf);

                        char nullch = ',';
                        //while (*((unsigned char*)addr) == 0) addr += 0x100;
                        for (int t = 0; t < 16; ++t, addr += 0x100) {
                            buf[0] = '\"';
                            for (int k = 0; k < 16; ++k) {
                                buf[k + 1] = *((unsigned char*)addr + k);
                                if (!buf[k + 1]) buf[k + 1] = nullch;
                                //sprintf(buf, "[%c](%02x)", *((unsigned char*)addr + k), *((unsigned char*)addr + k));
                            }
                            buf[17] = '\"';
                            buf[18] = '\0';
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                        }
                        sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x1000, addr, nullch);
                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.y = cons_newline(cursor.y, sheet);

                    } else if (!strncmp(cmdline, "ffmp ", 5)) {
                        //strcpy(buf, (char*)finfo);
                        //int i = 0;
                        //do {
                        //    ++i;
                        //    buf[i] = cmdline[i + 5];
                        //} while (cmdline[i]);

                        strcpy(buf, cmdline + 5);
                        int addr = atoi(buf);

                        char nullch = ',';
                        //while (*((unsigned char*)addr) == 0) addr += 0x100;
                        for (int t = 0; t < 16; ++t, addr += 0x1000) {
                            buf[0] = '\"';
                            for (int k = 0; k < 16; ++k) {
                                buf[k + 1] = *((unsigned char*)addr + k * 0x100);
                                if (!buf[k + 1]) buf[k + 1] = nullch;
                                //sprintf(buf, "[%c](%02x)", *((unsigned char*)addr + k), *((unsigned char*)addr + k));
                            }
                            buf[17] = '\"';
                            buf[18] = '\0';
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                        }
                        sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x10000, addr, nullch);
                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.y = cons_newline(cursor.y, sheet);

                    } else if (!strncmp(cmdline, "msearch ", 8)) {
                        //strcpy(buf, (char*)finfo);
                        //int i = 0;
                        //do {
                        //    ++i;
                        //    buf[i] = cmdline[i + 5];
                        //} while (cmdline[i]);

                        strcpy(buf, cmdline + 8);
                        int i = 0, j;
                        while (buf[i] != ' ') ++i;
                        buf[i] = '\0';
                        unsigned char word[16];
                        strcpy(word, buf);
                        for (j = 0; buf[i + j + 1]; ++j) buf[j] = buf[i + j + 1];
                        buf[j] = '\0';

                        int addr = atoi(buf), len = strlen(word);
                        char match;
                        int addr_end = 0x01000000;

                        for (; addr < addr_end; ++addr) {
                            match = TRUE;
                            for (i = 0; i < len && match; ++i) {
                                if (((unsigned char*)addr)[i] != word[i]) match = FALSE;
                            }
                            if (match) break;
                        }
                        if (match)
                            sprintf(buf, "word found at: %08x", addr);

                        else
                            sprintf(buf, "word not found by: %08x", addr_end);

                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.y = cons_newline(cursor.y, sheet);
                    } else if (cmdline[0]) {
                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, "invalid command.", 16);
                        cursor.y = cons_newline(cursor.y, sheet);
                        //cursor.y = cons_newline(cursor.y, sheet);
                    }

                    putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, ">", 1);
                    cursor.x = 1;
                } else {
                    if (cursor.x < width) {
                        buf[0] = data;
                        buf[1] = '\0';
                        cmdline[cursor.x - 1] = data;
                        putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.x++;
                    }
                }
            }
            if (cursor.col >= 0) putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_008484, cursor.col, " ", 1);
        }
    }
}

int cons_newline(int cursor_y, struct SHEET* sheet) {
    int width = (sheet->bxsize - 8 - 1) / 8 - 2, height = (sheet->bysize - 28 - 1) / 16 - 1;

    if (cursor_y < height - 1)
        cursor_y++;
    else {
        for (int y = 28; y < 28 + (height - 1) * 16; ++y) {
            for (int x = 8; x < 8 + (width - 1) * 8; ++x) {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (int y = 28 + (height - 1) * 16; y < 28 + height * 16; ++y) {
            for (int x = 8; x < 8 + (width - 1) * 8; ++x) {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + (width - 1) * 8, 28 + height * 16);
    }
    return cursor_y;
}