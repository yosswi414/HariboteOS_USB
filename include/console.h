#pragma once

#include "sheet.h"
#include "timer.h"

struct CONSOLE {
    struct SHEET* sht;
    int cur_x, cur_y;
    int cur_c;
    struct TIMER* timer;
    int off_x, off_y;
    int width, height;
};

void console_task(struct SHEET* sheet, int memtotal);
void cons_putchar(struct CONSOLE* cons, int chr, char move);
void cons_newline(struct CONSOLE* cons);
void cons_putstr0(struct CONSOLE* cons, const char* s);
void cons_putstr1(struct CONSOLE* cons, const char* s, size_t n);
void cons_runcmd(char* cmdline, struct CONSOLE* cons, int* fat, unsigned int memtotal);

void cmd_free(struct CONSOLE* cons, unsigned int memtotal);
void cmd_clear(struct CONSOLE* cons);
void cmd_ls(struct CONSOLE* cons);
int cmd_cat(struct CONSOLE* cons, int* fat, char* cmdline);
void cmd_dump(struct CONSOLE* cons, int addr);
void cmd_fump(struct CONSOLE* cons, int addr);
void cmd_msearch(struct CONSOLE* cons, int addr, char* word);
void cmd_mvsearch(struct CONSOLE* cons, int addr, uint val);
void cmd_mwrite(struct CONSOLE* cons, int addr, unsigned char val);
void cmd_debug(struct CONSOLE* cons);
int cmd_app(struct CONSOLE* cons, int* fat, char* cmdline);

int* hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
void hrb_api_linewin(struct SHEET* sht, int x0, int y0, int x1, int y1, int col);

int* inthandler00(int* esp);
int* inthandler06(int* esp);
int* inthandler0c(int* esp);
int* inthandler0d(int* esp);
