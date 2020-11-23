#include "api.h"
#include "graphic.h"

void HariMain(){
    char* buf;
    int win, x, y;
    api_initmalloc();
    buf = api_malloc(160 * 108);
    win = api_openwin(buf, 160, 108, -1, "walk");
    api_boxfillwin(win, 4, 24, 155, 103, COL8_000000);
    x = 9;
    y = 3;
    api_putstrwin(win, x * 8 + 4, y * 16 + 24, COL8_FFFF00, 1, "@");
    for (;;){
        int i = api_getkey(TRUE);
        api_putstrwin(win, x * 8 + 4, y * 16 + 24, COL8_000000, 1, "@");
        if (i == '4') x = max(x - 1, 0);
        if (i == '6') x = min(x + 1, 18);
        if (i == '8') y = max(y - 1, 0);
        if (i == '2') y = min(y + 1, 4);
        if (i == '\n') break;
        api_putstrwin(win, x * 8 + 4, y * 16 + 24, COL8_FFFF00, 1, "@");
    }
    api_closewin(win);
    api_end();
}