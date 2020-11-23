#include "api.h"
#include "graphic.h"

void HariMain() {
    char* buf;
    int win;

    api_initmalloc();
    buf = api_malloc(150 * 50);
    win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfillwin(win, 8, 36, 141, 43, COL8_00FFFF);
    api_putstrwin(win, 28, 28, COL8_000000, 12, "hello, world");
    api_getkey(TRUE);
    api_end();
}
