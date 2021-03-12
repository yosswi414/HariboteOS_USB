#include "apilib.h"

void HariMain(){
    int fh;
    char c;
    fh = api_fopen("console.c");
    if(fh)
        while (api_fread(&c, 1, fh)) api_putchar(c);
    else
        api_putstr0("console.c not found\n");
    api_end();
}