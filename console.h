#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "sheet.h"

struct COORD {
    int x, y;
    int col;
};

void console_task(struct SHEET* sheet, int memtotal);
int cons_newline(int cursor_y, struct SHEET* sheet);

#endif