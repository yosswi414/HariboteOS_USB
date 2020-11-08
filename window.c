#include "window.h"

#include "fifo.h"
#include "graphic.h"

void make_window8(unsigned char* buf, int xsize, int ysize, char* title, char act) {
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0, ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, title, act);
    return;
}

void make_wtitle8(unsigned char* buf, int xsize, char* title, char act) {
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"};
    char tc, tbc;
    if (act)
        tc = COL8_FFFFFF, tbc = COL8_000084;
    else
        tc = COL8_C6C6C6, tbc = COL8_848484;

    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    putfonts8(buf, xsize, 24, 4, tc, title);

    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 16; x++) {
            char c = closebtn[y][x];
            switch (c) {
                case '@':
                    c = COL8_000000;
                    break;
                case '$':
                    c = COL8_848484;
                    break;
                case 'Q':
                    c = COL8_C6C6C6;
                    break;
                default:
                    c = COL8_FFFFFF;
                    break;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

int keywin_off(struct SHEET* key_win, struct SHEET* sht_win, int cur_c, int cur_x) {
    change_wtitle8(key_win, FALSE);
    if (key_win == sht_win) {
        cur_c = -1;
        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, 8 * cur_x, 28, 8 * cur_x + 7, 43);
    } else {
        if (key_win->flags & SHEET_FLAGS_CURSOR) fifo32_put(&key_win->task->fifo, 3);  // turn cursor off
    }
    return cur_c;
}

int keywin_on(struct SHEET* key_win, struct SHEET* sht_win, int cur_c) {
    change_wtitle8(key_win, TRUE);
    if (key_win == sht_win)
        cur_c = COL8_000000;
    else {
        if (key_win->flags & SHEET_FLAGS_CURSOR) fifo32_put(&key_win->task->fifo, 2);  // turn cursor on
    }
    return cur_c;
}

void change_wtitle8(struct SHEET* sht, char act) {
    int xsize = sht->bxsize;
    char c, tc[2], tbc[2], *buf = sht->buf;
    act = !!act;
    tc[act] = COL8_FFFFFF;
    tc[!act] = COL8_C6C6C6;
    tbc[act] = COL8_000084;
    tbc[!act] = COL8_848484;
    for (int y = 3; y <= 20; ++y) {
        for (int x = 3; x <= xsize - 4; ++x) {
            c = buf[y * xsize + x];
            if (c == tc[0] && x <= xsize - 22)
                c = tc[1];
            else if (c == tbc[0])
                c = tbc[1];
            buf[y * xsize + x] = c;
        }
    }
    sheet_refresh(sht, 3, 3, xsize, 21);
    return;
}