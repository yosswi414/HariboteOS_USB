#include "apilib.h"
#include "graphic.h"
#include "mylibgcc.h"

void HariMain(){
    char buf[150 * 100];
    int win, x, y;
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