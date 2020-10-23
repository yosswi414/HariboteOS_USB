#include "api.h"
#include "graphic.h"

void HariMain(){
    char* buf;
    int win;
    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "star1");
    api_boxfillwin(win, 6, 26, 143, 93, COL8_000000);
    api_point(win, 75, 59, COL8_FFFF00);
    api_end();
}