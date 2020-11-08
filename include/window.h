#pragma once

#include "sheet.h"

void make_window8(unsigned char* buf, int xsize, int ysize, char* title, char act);
void make_wtitle8(unsigned char* buf, int xsize, char* title, char act);

int keywin_off(struct SHEET* key_win, struct SHEET* sht_win, int cur_c, int cur_x);
int keywin_on(struct SHEET* key_win, struct SHEET* sht_win, int cur_c);

void change_wtitle8(struct SHEET* sht, char act);