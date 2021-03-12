#include "apilib.h"
#include "general.h"

void HariMain(){
    int win;
    char buf[150 * 50];
    win = api_openwin(buf, 150, 50, -1, "hello1");
    while (api_getkey(TRUE) != '\n') {}
    api_end();
}