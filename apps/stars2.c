#include "apilib.h"
#include "graphic.h"

uint rand_xor32(void) {
    static uint y = 2463534242u;
    y = y ^ (y << 13);
    y = y ^ (y >> 17);
    return y = y ^ (y << 5);
}

void HariMain() {
    char* buf;
    int win, x, y;
    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "stars2");
    api_boxfillwin(win + 1, 6, 26, 143, 93, COL8_000000);
    for (int i = 0; i < 300; ++i) {
        x = rand_xor32() % 137 + 6;
        y = rand_xor32() % 67 + 26;
        api_point(win + 1, x, y, COL8_FFFF00);
    }
    api_refreshwin(win, 6, 26, 144, 94);
    api_getkey(TRUE);
    api_end();
}