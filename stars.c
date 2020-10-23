#include "api.h"
#include "graphic.h"

uint rand_xor32(void) {
    static uint y = 2463534242u;
    y = y ^ (y << 13);
    y = y ^ (y >> 17);
    return y = y ^ (y << 5);
}

void HariMain(){
    char* buf;
    int win, x, y;
    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "stars");
    api_boxfillwin(win, 6, 26, 143, 93, COL8_000000);
    int points = 1e9;
    for (int i = 0; i < points; ++i) {
        x = rand_xor32() % 138 + 6;
        y = rand_xor32() % 68 + 26;
        api_point(win, x, y, rand_xor32() % 16);
    }
    api_end();
}