#include "apilib.h"
#include "general.h"

void HariMain() {
    int win;
    char buf[150 * 50];
    win = api_openwin(buf, 150, 50, -1, "hello2");
    api_boxfillwin(win, 8, 36, 141, 43, 3); // yellow for 3
    api_putstrwin(win, 28, 28, 0, 12, "hell0, world"); // black for 0
    api_getkey(TRUE);
    api_end();
}