#pragma once

#include "sheet.h"

void make_window8(unsigned char* buf, int xsize, int ysize, char* title, char act);
void make_wtitle8(unsigned char* buf, int xsize, char* title, char act);

void keywin_off(struct SHEET* key_win);
void keywin_on(struct SHEET* key_win);

void change_wtitle8(struct SHEET* sht, char act);