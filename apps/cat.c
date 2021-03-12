#include "apilib.h"

#define MAX_CMDLEN 128

void HariMain(){
    int fh;
    char c, cmdline[MAX_CMDLEN], *p;

    api_cmdline(cmdline, MAX_CMDLEN);
    for (p = cmdline; *p > ' '; ++p) {}
    while (*p == ' ') ++p;
    fh = api_fopen(p);
    if(fh)
        while (api_fread(&c, 1, fh)) api_putchar(c);
    else
        api_putstr0("File not found.\n");
    
    api_end();
}