#include "api.h"
#include "graphic.h"
#include "mylibgcc.h"
#include "timer.h"

void HariMain(){
    char *buf, s[60];
    int win, timer, sec = 0, min = 0, hou = 0;
    api_initmalloc();
    buf = api_malloc(150 * 50);
    win = api_openwin(buf, 150, 50, -1, "noodle");
    timer = api_alloctimer();
    api_inittimer(timer, 128);
    for (;;){
        sprintf(s, "%5d:%02d:%02d", hou, min, sec);
        api_boxfillwin(win, 28, 27, 115, 41, COL8_FFFFFF);
        api_putstrwin(win, 28, 27, COL8_000000, 11, s);
        api_settimer(timer, 100); // 1 sec
        if (api_getkey(TRUE) != 128) break;
        ++sec;
        min += sec / 60, sec %= 60;
        hou += min / 60, min %= 60;
    }
    api_end();
}