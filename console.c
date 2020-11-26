#include "console.h"

#include "acpi.h"
#include "asmfunc.h"
#include "desctable.h"
#include "device.h"
#include "file.h"
#include "graphic.h"
#include "memory.h"
#include "mtask.h"
#include "mylibgcc.h"
#include "sheet.h"
#include "sysfunc.h"
#include "timer.h"
#include "window.h"

extern int dbg_val[4];
extern char dbg_str[4][64];
extern char isAcpiAvail;

struct CONSOLE* cons_dprintf;
char str_dprintf[64];
#define dprintf(...)                       \
    if (cons_dprintf) {                    \
        sprintf(str_dprintf, __VA_ARGS__); \
        cons_putstr0(cons_dprintf, str_dprintf);   \
    }

void console_task(struct SHEET* sheet, int memtotal) {
    struct TASK* task = task_now();
    char buf[128], cmdline[128];
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    // struct FILEINFO* finfo = (struct FILEINFO*)(ADDR_DISKIMG + 0x002600); // unused for now
    int* fat = (int*)memman_alloc_4k(memman, SIZE_FAT * sizeof(int));
    // struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)ADDR_GDT; // unused for now

    file_readfat(fat, (unsigned char*)(ADDR_DISKIMG + 0x000200));

    struct CONSOLE cons;
    cons.sht = sheet;
    cons.cur_c = -1;
    cons.cur_x = 0;
    cons.cur_y = 0;
    if (cons.sht) {
        cons.timer = timer_alloc();
        timer_init(cons.timer, &task->fifo, 1);
        timer_settime(cons.timer, 50);
    }
    cons.off_x = 8;
    cons.off_y = 28;
    cons.width = (cons.sht->bxsize - cons.off_x - 1) / 8 - 2;
    cons.height = (cons.sht->bysize - cons.off_y - 1) / 16 - 1;

    // usually cons.width depends on the size of console window.
    // when it comes to console-less mode, it should not be like above.
    // subtracting 5 itself has no definite reason or purpose.
    if (!cons.sht) cons.width = 128 - 5;

    // allow to access cons from anywhere
    //*((int*)ADDR_CONSOLE) = (int)&cons;
    task->cons = &cons;

    sprintf(buf, "bsize: (%d, %d)\n", cons.sht->bxsize, cons.sht->bysize);
    cons_putstr0(task->cons, buf);

    cons_putchar(&cons, '>', TRUE);

    dprintf("f, s: %x, %x\n", &task->fifo, cons.sht);

    while (TRUE) {
        io_cli();
        if (!fifo32_status(&task->fifo)) {
            task_sleep(task);
            io_sti();
        } else {
            int data = fifo32_get(&task->fifo);
            io_sti();
            if (!cons.sht) {
                dprintf("nc received: [%d](sht:%x)\n", data, cons.sht);
            }
            if (data <= 1) {  // timer for cursor
                if (cons.sht){
                    timer_init(cons.timer, &task->fifo, 1 - data);
                    if (cons.cur_c >= 0) cons.cur_c = data ? COL8_FFFFFF : COL8_000000;
                    timer_settime(cons.timer, 50);
                    //putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, COL8_008484, cons.cur_c, " ", 1);
                }
            }
            if (data == 2) cons.cur_c = COL8_FFFFFF;
            if (data == 3) { // cursor off
                cons.cur_c = -1;
                if (cons.sht){
                    boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, (cons.cur_x + 1) * 8 + cons.off_x - 1, (cons.cur_y + 1) * 16 + cons.off_y - 1);
                    //putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, COL8_008484, COL8_000000, " ", 1);
                }
            }
            if (data == 4) {
                cmd_exit(&cons, fat);
            }
            if (data == 5) {
                cmd_shutdown(&cons, isAcpiAvail);
            }
            if ((data & MASK_SIGNAL) == SIGNAL_KEY) {
                
                data &= ~SIGNAL_KEY;
                if (!cons.sht) dprintf("received: %c\n", data);
                if (data == '\b') {
                    if (cons.cur_x > 1) {
                        cons_putchar(&cons, ' ', FALSE);
                        cons.cur_x--;
                    }
                } else if (data == '\n') {
                    putfonts8_sht(cons.sht, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cons_putchar(&cons, ' ', FALSE);
                    cmdline[cons.cur_x - 1] = '\0';
                    //putfonts8_sht(sheet, 20, 20, COL8_FF0000, COL8_008484, cmdline, strlen(cmdline));
                    //cons.cur_x = 0;
                    //cons.cur_y = cons_newline(cons.cur_y, sheet);
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);
                    if (!cons.sht) {
                        dprintf("nc command: [%s]\n", cmdline);
                    }
                    if (!cons.sht) cmd_exit(&cons, fat);
                    cons_putchar(&cons, '>', TRUE);
                } else {
                    if (!cons.sht) dprintf("* ");
                    if (cons.cur_x + 1 < cons.width) {
                        if (!cons.sht) dprintf("$(%d) ", cons.cur_x);
                        //buf[0] = data;
                        //buf[1] = '\0';
                        cmdline[cons.cur_x - 1] = data;
                        //putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        //cons.cur_x++;
                        cons_putchar(&cons, data, TRUE);
                    }
                }
                if (!cons.sht) {
                    dprintf("nc command: [%s]\n", cmdline);
                }
            }
            if (cons.sht) {
                if (cons.cur_c >= 0) {
                    boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c,
                             cons.cur_x * 8 + cons.off_x,
                             cons.cur_y * 16 + cons.off_y,
                             (cons.cur_x + 1) * 8 + cons.off_x - 1,
                             (cons.cur_y + 1) * 16 + cons.off_y - 1);
                }
                sheet_refresh(cons.sht,
                              cons.cur_x * 8 + cons.off_x,
                              cons.cur_y * 16 + cons.off_y,
                              (cons.cur_x + 1) * 8 + cons.off_x,
                              (cons.cur_y + 1) * 16 + cons.off_y);
            }
        }
    }
}

void cons_putchar(struct CONSOLE* cons, int chr, char move) {
    char s[2];
    s[0] = chr;
    s[1] = '\0';
    switch (s[0]) {
        case '\t':
            do {
                if (cons->sht) putfonts8_sht(cons->sht, cons->cur_x * 8 + cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, " ", 1);
                cons->cur_x++;
                if (cons->cur_x >= cons->width) cons_newline(cons);
            } while (cons->cur_x % 4);
            break;
        case '\n':
            cons_newline(cons);
            break;
        case '\r':
            break;
        default:
            if (cons->cur_x >= cons->width) cons_newline(cons);
            if (cons->sht) putfonts8_sht(cons->sht, cons->cur_x * 8 + cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, s, strlen(s));
            if (move) {
                cons->cur_x++;
                if (cons->cur_x >= cons->width) cons_newline(cons);
            }
            break;
    }
    return;
}

void cons_newline(struct CONSOLE* cons) {
    if (cons->cur_y < cons->height - 1)
        cons->cur_y++;
    else {
        if (cons->sht) {
            for (int y = cons->off_y; y < cons->off_y + (cons->height - 1) * 16; ++y) {
                for (int x = cons->off_x; x < cons->off_x + cons->width * 8; ++x) {
                    cons->sht->buf[x + y * cons->sht->bxsize] = cons->sht->buf[x + (y + 16) * cons->sht->bxsize];
                }
            }
            for (int y = cons->off_y + (cons->height - 1) * 16; y < cons->off_y + cons->height * 16; ++y) {
                for (int x = cons->off_x; x < cons->off_x + cons->width * 8; ++x) {
                    cons->sht->buf[x + y * cons->sht->bxsize] = COL8_000000;
                }
            }
            sheet_refresh(cons->sht, cons->off_x, cons->off_y, cons->off_x + cons->width * 8, cons->off_y + cons->height * 16);
        }
    }
    cons->cur_x = 0;
    return;
}

void cons_putstr0(struct CONSOLE* cons, const char* s) {
    while (*s) cons_putchar(cons, *(s++), TRUE);
    return;
}

void cons_putstr1(struct CONSOLE* cons, const char* s, size_t n) {
    for (int i = 0; i < n; ++i) cons_putchar(cons, s[i], TRUE);
    return;
}

void cons_runcmd(char* cmdline, struct CONSOLE* cons, int* fat, unsigned int memtotal) {
    char exit_success = FALSE;
    char buf[80], word[16];

    sprintf(buf, "cmd_app verbose: cmdline: %s\n", cmdline);
    cons_putstr0(cons, buf);

    do {
        if (!cmdline[0]) {
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "mem") || !strcmp(cmdline, "free")) {
            if (cons->sht) cmd_free(cons, memtotal);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "clear") || !strcmp(cmdline, "cls")) {
            if (cons->sht) cmd_clear(cons);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "ls") || !strcmp(cmdline, "dir")) {
            if (cons->sht) cmd_ls(cons);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "dump ", 5)) {
            if (!cmdline[5]) break;
            strcpy(buf, cmdline + 5);
            int addr = atoi(buf);
            cmd_dump(cons, addr);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "fump ", 5)) {
            if (!cmdline[5]) break;
            strcpy(buf, cmdline + 5);
            int addr = atoi(buf);
            cmd_fump(cons, addr);
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
            strcpy(word, buf);
            for (j = 0; buf[i + j + 1]; ++j) buf[j] = buf[i + j + 1];
            buf[j] = '\0';
            if (!strlen(buf)) break;
            int addr = atoi(buf);

            cmd_msearch(cons, addr, word);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "mvsearch ", 9)) {
            if (!cmdline[9]) break;
            strcpy(buf, cmdline + 9);
            int i = 0, j;
            while (buf[i] != ' ') ++i;
            buf[i] = '\0';
            if (!strlen(buf)) break;
            strcpy(word, buf);
            for (j = 0; buf[i + j + 1]; ++j) buf[j] = buf[i + j + 1];
            buf[j] = '\0';
            if (!strlen(buf)) break;
            int addr = atoi(buf);

            cmd_mvsearch(cons, addr, atoi(word));
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
            int addr = atoi(buf);
            for (j = 0; buf[i + j + 1]; ++j) buf[j] = buf[i + j + 1];
            buf[j] = '\0';
            if (!strlen(buf)) break;
            unsigned char val = atoi(buf);

            cmd_mwrite(cons, addr, val);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "cat ", 4)) {
            if (cons->sht)
                exit_success = !cmd_cat(cons, fat, cmdline);
            else
                exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "debug")) {
            if (cons->sht) cmd_debug(cons);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "shutdown")) {
            cmd_shutdown(cons, isAcpiAvail);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "exit")) {
            cmd_exit(cons, fat);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "testi ", strlen("testi "))) {
            if (!cmdline[strlen("testi ")]) break;
            strcpy(buf, cmdline + strlen("testi "));
            int arg = atoi(buf);
            cmd_testi(cons, arg);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "testc ", strlen("testc "))) {
            if (!cmdline[strlen("testc ")]) break;
            strcpy(buf, cmdline + strlen("testc "));
            cmd_testc(cons, buf);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "start ", strlen("start "))) {
            cmd_start(cons, cmdline, memtotal);
            exit_success = TRUE;
            break;
        }
        if (!strncmp(cmdline, "ncst ", strlen("ncst "))) {
            cmd_ncst(cons, cmdline, memtotal);
            exit_success = TRUE;
            break;
        }
        exit_success = !cmd_app(cons, fat, cmdline);
    } while (FALSE);
    if (!exit_success) {
        cons_putstr0(cons, "invalid command.\n");
    }
    for (int i = 0; i < 80; ++i) buf[i] = '\0';
}

void cmd_free(struct CONSOLE* cons, unsigned int memtotal) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    char buf[64];
    sprintf(buf, "total %d KB\nfree  %d KB\n", memtotal / (1 << 10), memman_total(memman) / (1 << 10));
    cons_putstr0(cons, buf);
    return;
}

void cmd_clear(struct CONSOLE* cons) {
    for (int y = cons->off_y; y < cons->off_y + cons->height * 16; ++y)
        for (int x = cons->off_x; x < cons->off_x + cons->width * 8; ++x) cons->sht->buf[x + y * cons->sht->bxsize] = COL8_000000;
    sheet_refresh(cons->sht, cons->off_x, cons->off_y, cons->off_x + cons->width * 8, cons->off_y + cons->height * 16);
    cons->cur_y = 0;
    return;
}

void cmd_ls(struct CONSOLE* cons) {
    struct FILEINFO* finfo = (struct FILEINFO*)(ADDR_DISKIMG + 0x002600);
    char buf[64];

    for (int x = 0; x < 224; ++x) {
        if (finfo[x].name[0] == 0x00) {
            cons_putstr0(cons, "No further file found.\n");
            break;
        }
        if (finfo[x].name[0] != 0xe5) {
            if ((finfo[x].type & 0x18) == 0) {
                sprintf(buf, "filename.ext   %7d\n", finfo[x].size);
                strncpy(buf, finfo[x].name, 8);
                strncpy(buf + 9, finfo[x].ext, 3);

                // for (int y = 0; y < 8; y++) {
                //     buf[y] = finfo[x].name[y];
                // }
                // buf[9] = finfo[x].ext[0];
                // buf[10] = finfo[x].ext[1];
                // buf[11] = finfo[x].ext[2];

                cons_putstr0(cons, buf);
            }
        }
    }
}

int cmd_cat(struct CONSOLE* cons, int* fat, char* cmdline) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    int spcpos;
    if (!strncmp(cmdline, "cat ", 4))
        spcpos = 4;
    else if (!strncmp(cmdline, "type ", 5))
        spcpos = 5;
    else
        return 1;
    struct FILEINFO* finfo = file_search(cmdline + spcpos, (struct FILEINFO*)(ADDR_DISKIMG + 0x002600), 224);
    char* p;

    if (finfo) {
        // for debug
        dbg_val[0] = ADDR_DISKIMG + 0x3e00 + finfo->clustno * 512;
        sprintf(dbg_str[0], "cat: Head pointer of file.");

        p = (char*)memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char*)(ADDR_DISKIMG + 0x003e00));
        cons_putstr1(cons, p, finfo->size);
        memman_free_4k(memman, (int)p, finfo->size);
    } else {
        cons_putstr0(cons, "file not found\n");
    }
    cons_newline(cons);
    return 0;
}

void cmd_dump(struct CONSOLE* cons, int addr) {
    char nullch = '.';
    char text[19], code[8], buf[64];

    sprintf(buf, "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f");
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
    sprintf(buf, "-----------------------------------------------");
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);

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
        putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
        cons_newline(cons);
    }
    sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x100, addr, nullch);
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
}

void cmd_fump(struct CONSOLE* cons, int addr) {
    char nullch = '.';
    char text[19], code[8], buf[64];

    sprintf(buf, "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f");
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
    sprintf(buf, "-----------------------------------------------");
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);

    //while (*((unsigned char*)addr) == 0) addr += 0x100;
    for (int t = 0; t < 16; ++t, addr += 0x100) {
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
        putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
        cons_newline(cons);
    }
    sprintf(buf, "[begin: 0x%08x - end:0x%08x, null=\'%c\']", addr - 0x1000, addr, nullch);
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
}

void cmd_msearch(struct CONSOLE* cons, int addr, char* word) {
    int len = strlen(word);
    char match;
    int addr_end = 0x01000000;

    char buf[64];

    for (; addr < addr_end; ++addr) {
        match = TRUE;
        for (int i = 0; i < len && match; ++i) {
            if (((char*)addr)[i] != word[i]) match = FALSE;
        }
        if (match) break;
    }
    if (match)
        sprintf(buf, "word found at: %08x", addr);

    else
        sprintf(buf, "word not found by: %08x", addr_end);

    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
}

void cmd_mvsearch(struct CONSOLE* cons, int addr, uint val) {
    uint mask = 0xff;
    char match = FALSE;
    int addr_end = 0x01000000;

    if (val >= 0x100) mask = 0xffff;
    if (val >= 0x10000) mask = 0xffffff;
    if (val >= 0x1000000) mask = 0xffffffff;

    char buf[64];

    for (; addr < addr_end; ++addr) {
        if ((*((uint*)addr) & mask) == val) {
            match = TRUE;
            break;
        }
    }
    if (match)
        sprintf(buf, "word [0x%x] found at: %08x [0x%x]", val, addr, *((uint*)addr));

    else
        sprintf(buf, "word [0x%x] not found by: %08x", val, addr_end);

    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
    sprintf(buf, "mask: 0x%x", mask);
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
}

void cmd_mwrite(struct CONSOLE* cons, int addr, unsigned char val) {
    unsigned char old = *((unsigned char*)addr);
    char buf[64];

    *((unsigned char*)addr) = val;

    sprintf(buf, "data at %08x (%02x, \'%c\') -> (%02x, \'%c\').", addr, old, old, val, val);
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
}

void cmd_debug(struct CONSOLE* cons) {
    char buf[64];
    for (int i = 0; i < 4; ++i) {
        sprintf(buf, "dbg_val[%d] = 0x%08x (%d)", i, dbg_val[i], dbg_val[i]);
        putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
        cons_newline(cons);
        sprintf(buf, "dbg_str[%d] = \"", i);
        for (int j = 0; j < strlen(buf); ++j) cons_putchar(cons, buf[j], TRUE);
        for (int j = 0; j < min(64, strlen(dbg_str[i])); ++j) cons_putchar(cons, dbg_str[i][j], TRUE);
        cons_putchar(cons, '\"', TRUE);
        cons_newline(cons);
    }
    sprintf(buf, "No further debug info available.");
    putfonts8_sht(cons->sht, cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
    cons_newline(cons);
}

extern struct TASKCTL* taskctl;

void cmd_exit(struct CONSOLE* cons, int* fat) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct TASK* task = task_now();
    struct SHTCTL* shtctl = (struct SHTCTL*)*((int*)ADDR_SHTCTL);
    struct FIFO32* fifo = (struct FIFO32*)*((int*)ADDR_FIFO_TASK_A);
    if (cons->sht) timer_cancel(cons->timer);
    memman_free_4k(memman, (int)fat, SIZE_FAT * sizeof(int));
    io_cli();
    if (cons->sht)
        fifo32_put(fifo, (cons->sht - shtctl->sheets0) | SIGNAL_CONS_EXIT);
    else
        fifo32_put(fifo, (task - taskctl->tasks0) | SIGNAL_APP_EXIT);
    io_sti();
    while (TRUE) task_sleep(task);
}

void cmd_shutdown(struct CONSOLE* cons, char mode) {
    cons_putstr0(cons, "Shutting down...\n");
    if (!mode) {
        wrstr("ACPI is not available.\n");
        wrstr("Using VBox feature... ");
        io_out16(0x4004, 0x3400);  // this works only in VirtualBox environment
        wrstr("Failed.\n");
        wrstr("Using Bochs feature... ");
        io_out16(0xb004, 0x2000);  // this works only in Bochs and old QEMU environment
        wrstr("Failed.\n");
        wrstr("Using QEMU feature... ");
        io_out16(0x0604, 0x2000);  // this works only in QEMU environment
        wrstr("Failed.\n");
        wrstr("Shutdown sequence aborted.\n");
    } else {
        cons_putstr0(cons, "Using ACPI...\n");
        acpiPowerOff();
        //asm_exit();
    }
    cons_putstr0(cons, "Failed to shutdown.\n\n");
    return;
}

int cmd_app(struct CONSOLE* cons, int* fat, char* cmdline) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct FILEINFO* finfo;
    struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)ADDR_GDT;
    char name[18], *p, *q;
    struct TASK* task = task_now();
    struct SHTCTL* shtctl;
    struct SHEET* sht;
    int segsiz, datsiz, esp, dathrb;
    int i;

    for (i = 0; i < 13 && cmdline[i] > ' '; ++i) name[i] = cmdline[i];
    name[i] = '\0';
    strcpy(dbg_str[0], name);
    finfo = file_search(name, (struct FILEINFO*)(ADDR_DISKIMG + 0x002600), 7 * 32);
    if (!finfo && name[i - 1] != '.') {
        strcat(name, ".HRB");
        strcpy(dbg_str[1], name);
        finfo = file_search(name, (struct FILEINFO*)(ADDR_DISKIMG + 0x002600), 224);
    }
    if (finfo) {
        p = (char*)memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char*)(ADDR_DISKIMG + 0x003e00));
        if (finfo->size >= 36 && !strncmp(p + 4, "Hari", 4) && !*p) {
            segsiz = *((int*)(p + 0x0000));
            esp = *((int*)(p + 0x000c));
            datsiz = *((int*)(p + 0x0010));
            dathrb = *((int*)(p + 0x0014));
            q = (char*)memman_alloc_4k(memman, SIZE_APPMEM);
            task->ds_base = (int)q;
            set_segmdesc(task->ldt + 0, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
            set_segmdesc(task->ldt + 1, segsiz - 1, (int)q, AR_DATA32_RW + 0x60);
            for (int i = 0; i < datsiz; ++i) q[esp + i] = p[dathrb + i];
            // + 4 : means that this is segment number of LDT, not GDT
            start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));

            shtctl = (struct SHTCTL*)*((int*)ADDR_SHTCTL);
            for (int i = 0; i < MAX_SHEETS; ++i) {
                sht = &(shtctl->sheets0[i]);
                if ((sht->flags | SHEET_FLAGS_APP | SHEET_FLAGS_USE) == sht->flags && sht->task == task) {
                    sheet_free(sht);
                }
            }
            timer_cancelall(&task->fifo);
            memman_free_4k(memman, (int)q, segsiz);
        } else {
            cons_putstr0(cons, ".hrb file format error.\n");
        }
        memman_free_4k(memman, (int)p, finfo->size);
        cons_newline(cons);
        return 0;
    }
    return 1;
}

int* hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    struct TASK* task = task_now();
    int ds_base = task->ds_base;
    struct CONSOLE* cons = task->cons;
    struct SHTCTL* shtctl = (struct SHTCTL*)*((int*)ADDR_SHTCTL);
    struct SHEET* sht;
    struct FIFO32* sys_fifo = (struct FIFO32*)*((int*)ADDR_FIFO_TASK_A);
    volatile int* reg = &eax + 1;
    char buf[80];
    /* reg: the pointer next to EAX
    * reg[0] : EDI
    * reg[1] : ESI
    * reg[2] : EBP
    * reg[3] : ESP
    * reg[4] : EBX
    * reg[5] : EDX
    * reg[6] : ECX
    * reg[7] : EAX
    * 
    * volatile keyword makes it not to be removed by optimization
    * ref: https://www.uquest.co.jp/embedded/learning/lecture09.html
    */

    switch (edx) {
        case 1:
            cons_putchar(cons, eax & 0xff, TRUE);
            break;
        case 2:
            cons_putstr0(cons, (char*)ebx + ds_base);
            break;
        case 3:
            cons_putstr1(cons, (char*)ebx + ds_base, ecx);
            break;
        case 4:
            return &(task->tss.esp0);
        case 5:
            // open window
            sht = sheet_alloc(shtctl);
            sht->task = task;
            sht->flags |= SHEET_FLAGS_APP;
            sheet_setbuf(sht, (unsigned char*)ebx + ds_base, esi, edi, eax);
            make_window8((unsigned char*)ebx + ds_base, esi, edi, (char*)ecx + ds_base, 0);
            sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
            sheet_updown(sht, shtctl->top);  // on task_a
            reg[7] = (int)sht;               // removed when -O2
            break;
        case 6:
            sht = (struct SHEET*)(ebx & 0xfffffffe);
            putfonts8(sht->buf, sht->bxsize, esi, edi, eax, (char*)ebp + ds_base);
            if (~ebx & 1) sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
            break;
        case 7:
            // rect draw api for window
            sht = (struct SHEET*)(ebx & 0xfffffffe);
            boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
            if (~ebx & 1) sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
            break;
        case 8:
            // memman init
            memman_init((struct MEMMAN*)(ebx + ds_base));
            ecx &= 0xfffffff0;
            memman_free((struct MEMMAN*)(ebx + ds_base), eax, ecx);
            break;
        case 9:
            // malloc
            ecx = (ecx + 0x0f) & 0xfffffff0;
            reg[7] = memman_alloc((struct MEMMAN*)(ebx + ds_base), ecx);
            break;
        case 10:
            // free
            ecx = (ecx + 0x0f) & 0xfffffff0;
            memman_free((struct MEMMAN*)(ebx + ds_base), eax, ecx);
            break;
        case 11:
            // draw a dot
            sht = (struct SHEET*)(ebx & 0xfffffffe);
            sht->buf[sht->bxsize * edi + esi] = eax;
            if (~ebx & 1) sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
            break;
        case 12:
            // refresh
            sht = (struct SHEET*)ebx;
            sheet_refresh(sht, eax, ecx, esi, edi);
            break;
        case 13:
            // draw a line
            sht = (struct SHEET*)(ebx & ~1);
            hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
            if (~ebx & 1) sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
            break;
        case 14:
            // close a window
            sheet_free((struct SHEET*)ebx);
            break;
        case 15:
            // input a key (break if receives 127 (from bootpack.c))
            for (int i; i != 127;) {
                io_cli();
                if (!fifo32_status(&task->fifo)) {
                    if (eax)
                        task_sleep(task);
                    else {
                        io_sti();
                        reg[7] = -1;
                        break;
                    }
                }
                i = fifo32_get(&task->fifo);
                io_sti();
                switch (i) {
                    // timer for cursor
                    case 0:
                    case 1:
                        // send 1 since there is no cursor while running app
                        timer_init(cons->timer, &task->fifo, 1);
                        timer_settime(cons->timer, 50);
                        break;
                    case 2:  // cursor: ON
                        cons->cur_c = COL8_FFFFFF;
                        break;
                    case 3:  // cursor: OFF
                        cons->cur_c = -1;
                        break;
                    case 4:
                        timer_cancel(cons->timer);
                        io_cli();
                        fifo32_put(sys_fifo, (cons->sht - shtctl->sheets0) | SIGNAL_CONS_EXIT_LEAVING_APP);
                        cons->sht = NULL;
                        io_sti();
                }
                if (i >= SIGNAL_KEY) {  // keyboard signal
                    reg[7] = i & ~SIGNAL_KEY;
                    break;
                }
            }
            break;
        case 16:
            // allocate timer
            reg[7] = (int)timer_alloc();
            ((struct TIMER*)reg[7])->flags |= TIMER_FLAGS_ISAPP;
            break;
        case 17:
            // initialize timer
            timer_init((struct TIMER*)ebx, &task->fifo, eax | SIGNAL_KEY);
            break;
        case 18:
            // set timer
            timer_settime((struct TIMER*)ebx, eax);
            break;
        case 19:
            // free timer
            timer_free((struct TIMER*)ebx);
            break;
        case 20:
            // beep
            if (!eax) {
                // turn off beep
                int i = io_in8(0x61);
                io_out8(0x61, i & 0x0d);
                sprintf(buf, "beep off(%d)\n", eax);
                wrstr(buf);
            } else {
                int i = 1193180000 / eax;
                io_out8(0x43, 0xb6);
                io_out8(0x42, i & 0xff);
                io_out8(0x42, i >> 8);

                sprintf(buf, "beep on(%d)(%d mHz?)\n", eax, i);
                wrstr(buf);

                // turn on beep
                i = io_in8(0x61);
                io_out8(0x61, (i | 0x03) & 0x0f);
            }
            break;
        default:
            dbg_val[0] = edx;
            sprintf(dbg_str[0], "\nUNKNOWN EDX CODE DETECTED.\n");
            cons_putstr0(cons, dbg_str[0]);
            sprintf(dbg_str[0], "EDX code");
    }
    return NULL;
}

void hrb_api_linewin(struct SHEET* sht, int x0, int y0, int x1, int y1, int col) {
    int x, y, len, dx, dy;
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    x = x0 << 10;
    y = y0 << 10;
    if (dx >= dy) {
        len = dx + 1;
        dx = (x0 > x1 ? -1 : 1) << 10;
        dy = ((y1 - y0 + (y0 > y1 ? -1 : 1)) << 10) / len;
    } else {
        len = dy + 1;
        dy = (y0 > y1 ? -1 : 1) << 10;
        dx = ((x1 - x0 + (x0 > x1 ? -1 : 1)) << 10) / len;
    }
    for (int i = 0; i < len; ++i) {
        sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
        x += dx;
        y += dy;
    }
    return;
}

int* inthandler00(int* esp) {
    struct TASK* task = task_now();
    struct CONSOLE* cons = task->cons;
    char s[32];
    cons_putstr0(cons, "\nINT 0x00 :\nZero Division Exception.\n");
    sprintf(s, "EIP = 0x%08X\n", esp[11]);
    cons_putstr0(cons, s);
    cons_putstr0(cons, "\nThe current process will be terminated.\n");
    return &(task->tss.esp0);
}

int* inthandler06(int* esp) {
    struct TASK* task = task_now();
    struct CONSOLE* cons = task->cons;
    char s[32];
    cons_putstr0(cons, "\nINT 0x06 :\nInvalid Instruction Exception.\n");
    sprintf(s, "EIP = 0x%08X\n", esp[11]);
    cons_putstr0(cons, s);
    cons_putstr0(cons, "\nThe current process will be terminated.\n");
    return &(task->tss.esp0);
}

int* inthandler0c(int* esp) {
    struct TASK* task = task_now();
    struct CONSOLE* cons = task->cons;
    char s[32];
    cons_putstr0(cons, "\nINT 0x0C :\nStack Exception.\n");
    sprintf(s, "EIP = 0x%08X\n", esp[11]);
    cons_putstr0(cons, s);
    cons_putstr0(cons, "\nThe current process will be terminated.\n");
    return &(task->tss.esp0);
}

int* inthandler0d(int* esp) {
    struct TASK* task = task_now();
    struct CONSOLE* cons = task->cons;
    char s[32];
    cons_putstr0(cons, "\nINT 0x0D :\nGeneral Protected Exception.\n");
    sprintf(s, "EIP = 0x%08X\n", esp[11]);
    cons_putstr0(cons, s);
    cons_putstr0(cons, "\nThe current process will be terminated.\n");
    return &(task->tss.esp0);
}

void cmd_testi(struct CONSOLE* cons, int arg) {
    struct FIFO32* fifo = ((struct FIFO32*)0x51e8);
    switch (arg) {
        case 1:
            cons_dprintf = cons;
            dprintf("cons set successfully. (cons: %x)\n", cons_dprintf);
            return;
        case 2:
            dprintf("status: %d\n", fifo32_status(fifo));
            return;
        default:
            dprintf("&%d->fifo:\t%x\n", arg, &((struct TASK*)arg)->fifo);
            dprintf(" %d->fifo:\t%x\n", arg, ((struct TASK*)arg)->fifo);
            return;
    }
}

void cmd_testc(struct CONSOLE* cons, char* arg){
    struct FIFO32* fifo = ((struct FIFO32*)0x51e8);
    int n = strlen(arg);
    char data;
    dprintf("fifo:[%x]\n", fifo);
    dprintf("arg:[");
    dprintf(arg);
    dprintf("](%d)\n", n);
    for (int i = 0; i < n;++i){
        if(arg[i]=='\\'){
            ++i;
            if (arg[i] == 'n') {
                data = '\n';
            }
            if (arg[i] == 't') {
                data = '\t';
            }
        }
        else
            data = arg[i];
        fifo32_put(fifo, data | SIGNAL_KEY);
        dprintf("send to fifo: %d | SIG = %d\n", data, data | SIGNAL_KEY);
    }
}

struct TASK* open_constask(struct SHEET* sht, uint memtotal) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct TASK* task = task_alloc();
    uint size_fifo = SIZE_FIFO_CONS;
    int* cons_fifo = (int*)memman_alloc_4k(memman, size_fifo * sizeof(int));
    task->cons_stack = memman_alloc_4k(memman, SIZE_APPMEM);
    task->tss.esp = task->cons_stack + SIZE_APPMEM - 12;
    task->tss.eip = (int)&console_task;
    task->tss.es = 1 * 8;
    task->tss.cs = 2 * 8;
    task->tss.ss = 1 * 8;
    task->tss.ds = 1 * 8;
    task->tss.fs = 1 * 8;
    task->tss.gs = 1 * 8;
    // (task->tss.eip): console_task arguments
    *((int*)(task->tss.esp + 4)) = (int)sht;  // 1st arg
    *((int*)(task->tss.esp + 8)) = memtotal;  // 2nd arg
    task_run(task, 2, 2);
    fifo32_init(&task->fifo, size_fifo, cons_fifo, task);
    dprintf("constask opened\n");
    dprintf("sht:\t%x\n", sht);
    dprintf("task:\t%x\n", task);
    if (task) {
        dprintf("L flags:\t");
        switch(task->flags){
            case TASK_STATE_STOPPED:
                dprintf("STOPPED\n");
                break;
            case TASK_STATE_WAITING:
                dprintf("WAITING\n");
                break;
            case TASK_STATE_RUNNING:
                dprintf("RUNNING\n");
                break;
        }
        dprintf("L &fifo:\t%x\n", &task->fifo);
        dprintf("L  fifo:\t%x\n", task->fifo);
    }
    return task;
}

void close_constask(struct TASK* task) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    task_sleep(task);
    memman_free_4k(memman, task->cons_stack, SIZE_APPMEM);
    memman_free_4k(memman, (int)task->fifo.buf, SIZE_FIFO_CONS * sizeof(int));
    task->flags = 0;
    dprintf("constask closed (task: %x)\n", task);
    return;
}

struct SHEET* open_console(struct SHTCTL* shtctl, uint memtotal, uint height, uint width) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct SHEET* sht = sheet_alloc(shtctl);
    uint wsize_x = width, wsize_y = height;
    unsigned char* buf = (unsigned char*)memman_alloc_4k(memman, wsize_x * wsize_y);
    sheet_setbuf(sht, buf, wsize_x, wsize_y, -1);  // no transparent color
    make_window8(buf, wsize_x, wsize_y, "console", FALSE);
    make_textbox8(sht, PADDING_LEFT, PADDING_ABOVE, wsize_x - PADDING_LEFT - PADDING_RIGHT, wsize_y - PADDING_ABOVE - PADDING_DOWN, COL8_000000);
    sht->task = open_constask(sht, memtotal);
    sht->flags |= SHEET_FLAGS_CURSOR;
    return sht;
}

void close_console(struct SHEET* sht) {
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct TASK* task = sht->task;
    memman_free_4k(memman, (int)sht->buf, sht->bxsize * sht->bysize);
    sheet_free(sht);
    close_constask(task);
    return;
}

void cmd_start(struct CONSOLE* cons, char* cmdline, int memtotal) {
    struct SHTCTL* shtctl = (struct SHTCTL*)*((int*)ADDR_SHTCTL);
    struct SHEET* sht = open_console(shtctl, memtotal, 384, 165);
    struct FIFO32* fifo = &sht->task->fifo;
    sheet_slide(sht, 300, 300);
    sheet_updown(sht, shtctl->top);
    for (int i = strlen("start "); cmdline[i]; ++i)
        fifo32_put(fifo, cmdline[i] | SIGNAL_KEY);
    fifo32_put(fifo, '\n' | SIGNAL_KEY);
    cons_newline(cons);
    return;
}

void cmd_ncst(struct CONSOLE* cons, char* cmdline, int memtotal) {
    struct TASK* task = open_constask(NULL, memtotal);
    struct FIFO32* fifo = &task->fifo;
    char buf[32] = {0};
    dprintf("cmd_ncst fifo:%x\n", fifo);
    for (int i = strlen("ncst "); cmdline[i]; ++i) {
        fifo32_put(fifo, cmdline[i] | SIGNAL_KEY);
    }
    fifo32_put(fifo, '\n' | SIGNAL_KEY);
    dprintf("Status: %d\n", fifo32_status(fifo));
    cons_newline(cons);
    return;
}