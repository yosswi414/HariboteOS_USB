#include "apilib.h"
#include "general.h"

char buf[150 * 50];

void HariMain(){
    int win;
    win = api_openwin(buf, 150, 50, -1, "hello1");
    api_getkey(TRUE);
    api_end();
}