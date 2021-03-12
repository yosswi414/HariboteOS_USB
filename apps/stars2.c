#include "apilib.h"
#include "graphic.h"
#include "mylibgcc.h"

void HariMain() {
    char buf[150 * 100];
    int win, x, y;
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