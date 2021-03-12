#include "apilib.h"
#include "general.h"

void HariMain(){
    char buf[160 * 100];
    int win;
    win = api_openwin(buf, 160, 100, -1, "lines");
    for (int i = 0; i < 8;++i){
        api_linewin(win + 1, 8, 26, 77, i * 9 + 26, i);
        api_linewin(win + 1, 88, 26, i * 9 + 88, 89, i);
    }
    api_refreshwin(win, 6, 26, 154, 90);
    api_putstr0("Press Enter to quit...\n");
    while (api_getkey(TRUE) != 0x0a) { }
    api_closewin(win);
    api_end();
}