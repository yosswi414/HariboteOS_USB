#include "console.h"
#include "asmfunc.h"
#include "desctable.h"
#include "device.h"
#include "file.h"
#include "graphic.h"
#include "memory.h"
#include "mtask.h"
#include "mylibgcc.h"
#include "sheet.h"
#include "timer.h"

void console_task(struct SHEET* sheet, int memtotal) {
    struct TIMER* timer;
    struct TASK* task = task_now();
    char buf[128], cmdline[30];
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct FILEINFO* finfo = (struct FILEINFO*)(ADDR_DISKIMG + 0x002600);
    int* fat = (int*)memman_alloc_4k(memman, 4 * 2880);
    struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)ADDR_GDT;

    int width = (sheet->bxsize - 8 - 1) / 8 - 2, height = (sheet->bysize - 28 - 1) / 16 - 1;

    int fifobuf[128];
    fifo32_init(&task->fifo, sizeof(fifobuf) / sizeof(int), fifobuf, task);
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    file_readfat(fat, (unsigned char*)(ADDR_DISKIMG + 0x000200));

    struct COORD cursor;
    cursor.col = -1;
    cursor.x = 1;
    cursor.y = 0;

    putfonts8_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

    while (TRUE) {
        io_cli();
        if (!fifo32_status(&task->fifo)) {
            task_sleep(task);
            io_sti();
        } else {
            int data = fifo32_get(&task->fifo);
            io_sti();
            if (data <= 1) { // timer for cursor
                timer_init(timer, &task->fifo, 1 - data);
                if (cursor.col >= 0) cursor.col = data ? COL8_FFFFFF : COL8_000000;
                timer_settime(timer, 50);
                putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_008484, cursor.col, " ", 1);
            }
            if (data == 2) cursor.col = COL8_FFFFFF;
            if (data == 3) {
                cursor.col = -1;
                putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_008484, COL8_000000, " ", 1);
            }
            if (data & KEYSIG_BIT) {
                data &= ~KEYSIG_BIT;
                if (data == '\b') {
                    if (cursor.x > 1) {
                        putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, " ", 1);
                        cursor.x--;
                    }
                } else if (data == '\n') {
                    putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, " ", 1);
                    cmdline[cursor.x - 1] = '\0';
                    //putfonts8_sht(sheet, 20, 20, COL8_FF0000, COL8_008484, cmdline, strlen(cmdline));
                    cursor.x = 0;
                    cursor.y = cons_newline(cursor.y, sheet);

                    char exit_success = TRUE;
                    if (cmdline[0]) exit_success = FALSE;

                    while (cmdline[0]) {
                        if (!strcmp(cmdline, "mem") || !strcmp(cmdline, "free")) {
                            sprintf(buf, "total %d MB", memtotal / (1 << 20));
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            sprintf(buf, "free  %d KB", memman_total(memman) / (1 << 10));
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            //cursor.y = cons_newline(cursor.y, sheet);
                            exit_success = TRUE;
                            break;
                        }
                        if (!strcmp(cmdline, "clear") || !strcmp(cmdline, "cls")) {
                            for (int y = 28; y < 28 + height * 16; ++y)
                                for (int x = 8; x < 8 + width * 8; ++x) sheet->buf[x + y * sheet->bxsize] = COL8_000000;
                            sheet_refresh(sheet, 8, 28, 8 + width * 8, 28 + height * 16);
                            cursor.y = 0;
                            exit_success = TRUE;
                            break;
                        }
                        if (!strcmp(cmdline, "ls") || !strcmp(cmdline, "dir")) {
                            for (int x = 0; x < 224; ++x) {
                                if (finfo[x].name[0] == 0x00) {
                                    sprintf(buf, "No further file found.");
                                    putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                    cursor.y = cons_newline(cursor.y, sheet);
                                    break;
                                }
                                if (finfo[x].name[0] != 0xe5) {
                                    if ((finfo[x].type & 0x18) == 0) {
                                        sprintf(buf, "filename.ext   %7d", finfo[x].size);
                                        for (int y = 0; y < 8; y++) {
                                            buf[y] = finfo[x].name[y];
                                        }
                                        buf[9] = finfo[x].ext[0];
                                        buf[10] = finfo[x].ext[1];
                                        buf[11] = finfo[x].ext[2];
                                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                        cursor.y = cons_newline(cursor.y, sheet);
                                    }
                                }
                            }
                            exit_success = TRUE;
                            break;
                        }
                        if (!strncmp(cmdline, "dump ", 5)) {
                            if (!cmdline[5]) break;
                            strcpy(buf, cmdline + 5);
                            int addr = atoi(buf);

                            char nullch = '.';
                            char text[19], code[8];

                            sprintf(buf, "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f");
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            sprintf(buf, "-----------------------------------------------");
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);

                            //while (*((unsigned char*)addr) == 0) addr += 0x100;
                            for (int t = 0; t < 16; ++t, addr += 0x10) {
                                text[0] = '\"';
                                buf[0] = '\0';
                                for (int k = 0; k < 16; ++k) {
                                    text[k + 1] = *((unsigned char*)addr + k);
                                    sprintf(code, "%02x ", (unsigned char)text[k + 1]);
                                    strcat(buf, code);
                                    if (!text[k + 1]) text[k + 1] = nullch;
                                    //sprintf(buf, "[%c](%02x)", *((unsigned char*)addr + k), *((unsigned char*)addr + k));
                                }
                                text[17] = '\"';
                                text[18] = '\0';
                                strcat(buf, text);
                                putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                cursor.y = cons_newline(cursor.y, sheet);
                            }
                            sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x100, addr, nullch);
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            exit_success = TRUE;
                            break;
                        }
                        if (!strncmp(cmdline, "fump ", 5)) {
                            if (!cmdline[5]) break;
                            strcpy(buf, cmdline + 5);
                            int addr = atoi(buf);

                            char nullch = '.';
                            //while (*((unsigned char*)addr) == 0) addr += 0x100;
                            for (int t = 0; t < 16; ++t, addr += 0x100) {
                                buf[0] = '\"';
                                for (int k = 0; k < 16; ++k) {
                                    buf[k + 1] = *((unsigned char*)addr + k);
                                    if (!buf[k + 1]) buf[k + 1] = nullch;
                                    //sprintf(buf, "[%c](%02x)", *((unsigned char*)addr + k), *((unsigned char*)addr + k));
                                }
                                buf[17] = '\"';
                                buf[18] = '\0';
                                putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                cursor.y = cons_newline(cursor.y, sheet);
                            }
                            sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x1000, addr, nullch);
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            exit_success = TRUE;
                            break;
                        }
                        if (!strncmp(cmdline, "ffmp ", 5)) {
                            if (!cmdline[5]) break;
                            strcpy(buf, cmdline + 5);
                            int addr = atoi(buf);

                            char nullch = '.';
                            //while (*((unsigned char*)addr) == 0) addr += 0x100;
                            for (int t = 0; t < 16; ++t, addr += 0x1000) {
                                buf[0] = '\"';
                                for (int k = 0; k < 16; ++k) {
                                    buf[k + 1] = *((unsigned char*)addr + k * 0x100);
                                    if (!buf[k + 1]) buf[k + 1] = nullch;
                                    //sprintf(buf, "[%c](%02x)", *((unsigned char*)addr + k), *((unsigned char*)addr + k));
                                }
                                buf[17] = '\"';
                                buf[18] = '\0';
                                putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                cursor.y = cons_newline(cursor.y, sheet);
                            }
                            sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x10000, addr, nullch);
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            exit_success = TRUE;
                            break;
                        }
                        if (!strncmp(cmdline, "msearch ", 8)) {
                            if (!cmdline[8]) break;
                            strcpy(buf, cmdline + 8);
                            int i = 0, j;
                            while (buf[i] != ' ') ++i;
                            buf[i] = '\0';
                            if (!strlen(buf)) break;
                            unsigned char word[16];
                            strcpy(word, buf);
                            for (j = 0; buf[i + j + 1]; ++j) buf[j] = buf[i + j + 1];
                            buf[j] = '\0';
                            if (!strlen(buf)) break;

                            int addr = atoi(buf), len = strlen(word);
                            char match;
                            int addr_end = 0x01000000;

                            for (; addr < addr_end; ++addr) {
                                match = TRUE;
                                for (i = 0; i < len && match; ++i) {
                                    if (((unsigned char*)addr)[i] != word[i]) match = FALSE;
                                }
                                if (match) break;
                            }
                            if (match)
                                sprintf(buf, "word found at: %08x", addr);

                            else
                                sprintf(buf, "word not found by: %08x", addr_end);

                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);

                            exit_success = TRUE;
                            break;
                        }
                        if (!strncmp(cmdline, "mwrite ", 7)) {
                            if (!cmdline[7]) break;
                            strcpy(buf, cmdline + 7);
                            int i = 0, j;
                            while (buf[i] != ' ') ++i;
                            buf[i] = '\0';
                            if (!strlen(buf)) break;
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            int addr = atoi(buf);
                            for (j = 0; buf[i + j + 1]; ++j) buf[j] = buf[i + j + 1];
                            buf[j] = '\0';
                            if (!strlen(buf)) break;
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);

                            unsigned char val = atoi(buf), old = *((unsigned char*)addr);

                            *((unsigned char*)addr) = val;

                            sprintf(buf, "data at %08x (%02x, \'%c\') -> (%02x, \'%c\').", addr, old, old, val, val);
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);

                            exit_success = TRUE;
                            break;
                        }
                        if (!strncmp(cmdline, "cat ", 4)) {
                            if (!cmdline[4]) break;
                            strcpy(buf, cmdline + 4);
                            char name[12], word[32];
                            for (int i = 0; i < 11; ++i) name[i] = ' ';
                            name[11] = '\0';
                            for (int i = 0, j = 0; i < strlen(buf) && j < 11; ++i, ++j) {
                                if (buf[i] == '.') {
                                    ++i;
                                    j = max(j, 8);
                                }
                                name[j] = toupper(buf[i]);
                            }
                            int x;
                            for (x = 0; x < 224 && finfo[x].name[0]; ++x) {
                                if (!(finfo[x].type & 0x18) && !strncmp(finfo[x].name, name, 11)) break;
                            }
                            if (x < 224 && finfo[x].name[0]) {
                                char* p = (char*)memman_alloc_4k(memman, finfo[x].size);
                                file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char*)(ADDR_DISKIMG + 0x003e00));

                                /*
                                sprintf(buf, "[START]");
                                putfonts8_sht(sheet, 8 + cursor.x * 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                cursor.y = cons_newline(cursor.y, sheet);
                                */
                                for (int i = 0; i < finfo[x].size; ++i) {

                                    buf[0] = p[i];
                                    if (buf[0] == '\r') {
                                    } else if (buf[0] == '\n') {
                                        cursor.y = cons_newline(cursor.y, sheet);
                                        cursor.x = 0;
                                    } else if (buf[0] == '\t') {
                                        cursor.x += (4 - cursor.x % 4) % 4;
                                    } else {
                                        buf[1] = '\0';
                                        putfonts8_sht(sheet, 8 + cursor.x * 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                        cursor.x++;
                                    }
                                    if (cursor.x >= width) {
                                        cursor.y = cons_newline(cursor.y, sheet);
                                        cursor.x = 0;
                                    }
                                }
                                /*
                                sprintf(buf, "[EOF]");
                                putfonts8_sht(sheet, 8 + cursor.x * 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                cursor.y = cons_newline(cursor.y, sheet);
                                sprintf(buf, "[begin: 0x%08x, end: 0x%08x]", paddr, paddr + finfo[x].size);
                                putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                                */
                                memman_free_4k(memman, (int)p, finfo[x].size);
                                cursor.y = cons_newline(cursor.y, sheet);
                            } else {
                                putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, "file not found", 14);
                                cursor.y = cons_newline(cursor.y, sheet);
                            }
                            exit_success = TRUE;
                            break;
                        }
                        if (!strcmp(cmdline, "debug")) {
                            sprintf(buf, "No debug info available.");
                            putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                            cursor.y = cons_newline(cursor.y, sheet);
                            exit_success = TRUE;
                            break;
                        }
                        if (!strcmp(cmdline, "hlt")) {
                            sprintf(buf, "HLT     HRB");
                            int x;
                            for (x = 0; x < 224; ++x) {
                                if (!finfo[x].name[0]) break;
                                if (!(finfo[x].type & 0x18)) {
                                    if (!strncmp(finfo[x].name, buf, 11)) break;
                                }
                            }
                            if (x < 224 && finfo[x].name[0]) {
                                char* p = (char*)memman_alloc_4k(memman, finfo[x].size);
                                file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char*)(ADDR_DISKIMG + 0x003e00));
                                set_segmdesc(gdt + 1003, finfo[x].size - 1, (int)p, AR_CODE32_ER);
                                farjmp(0, 1003 * 8);
                                memman_free_4k(memman, (int)p, finfo[x].size);
                            } else {
                                putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, "file not found", 14);
                                cursor.y = cons_newline(cursor.y, sheet);
                            }

                            exit_success = TRUE;
                            break;
                        }
                        exit_success = FALSE;
                        break;
                    }
                    if (!exit_success) {
                        putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, "invalid command.", 16);
                        cursor.y = cons_newline(cursor.y, sheet);
                        //cursor.y = cons_newline(cursor.y, sheet);
                    }

                    putfonts8_sht(sheet, 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, ">", 1);
                    cursor.x = 1;
                } else {
                    if (cursor.x < width) {
                        buf[0] = data;
                        buf[1] = '\0';
                        cmdline[cursor.x - 1] = data;
                        putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        cursor.x++;
                    }
                }
            }
            if (cursor.col >= 0) putfonts8_sht(sheet, cursor.x * 8 + 8, cursor.y * 16 + 28, COL8_008484, cursor.col, " ", 1);
        }
    }
}

int cons_newline(int cursor_y, struct SHEET* sheet) {
    int width = (sheet->bxsize - 8 - 1) / 8 - 1, height = (sheet->bysize - 28 - 1) / 16 - 1;

    if (cursor_y < height - 1)
        cursor_y++;
    else {
        for (int y = 28; y < 28 + (height - 1) * 16; ++y) {
            for (int x = 8; x < 8 + (width - 1) * 8; ++x) {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (int y = 28 + (height - 1) * 16; y < 28 + height * 16; ++y) {
            for (int x = 8; x < 8 + (width - 1) * 8; ++x) {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + (width - 1) * 8, 28 + height * 16);
    }
    return cursor_y;
}
