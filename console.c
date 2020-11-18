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
#include "window.h"
#include "acpi.h"
#include "sysfunc.h"

extern int dbg_val[4];
extern char dbg_str[4][64];
extern char isAcpiAvail;

void console_task(struct SHEET* sheet, int memtotal) {
    struct TASK* task = task_now();
    char buf[128], cmdline[128];
    struct MEMMAN* memman = (struct MEMMAN*)MEMMAN_ADDR;
    struct FILEINFO* finfo = (struct FILEINFO*)(ADDR_DISKIMG + 0x002600);
    int* fat = (int*)memman_alloc_4k(memman, 4 * 2880);
    struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)ADDR_GDT;

    file_readfat(fat, (unsigned char*)(ADDR_DISKIMG + 0x000200));

    struct CONSOLE cons;
    cons.sht = sheet;
    cons.cur_c = -1;
    cons.cur_x = 0;
    cons.cur_y = 0;
    cons.timer = timer_alloc();
    timer_init(cons.timer, &task->fifo, 1);
    timer_settime(cons.timer, 50);
    cons.off_x = 8;
    cons.off_y = 28;
    cons.width = (cons.sht->bxsize - cons.off_x - 1) / 8 - 2;
    cons.height = (cons.sht->bysize - cons.off_y - 1) / 16 - 1;

    // allow to access cons from anywhere
    //*((int*)ADDR_CONSOLE) = (int)&cons;
    task->cons = &cons;

    cons_putchar(&cons, '>', TRUE);

    while (TRUE) {
        io_cli();
        sprintf(buf, "cur:(%d, %d)", cons.cur_x, cons.cur_y);
        if (!fifo32_status(&task->fifo)) {
            task_sleep(task);
            io_sti();
        } else {
            int data = fifo32_get(&task->fifo);
            io_sti();
            if (data <= 1) {  // timer for cursor
                timer_init(cons.timer, &task->fifo, 1 - data);
                if (cons.cur_c >= 0) cons.cur_c = data ? COL8_FFFFFF : COL8_000000;
                timer_settime(cons.timer, 50);
                //putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, COL8_008484, cons.cur_c, " ", 1);
            }
            if (data == 2) cons.cur_c = COL8_FFFFFF;
            if (data == 3) {
                cons.cur_c = -1;
                boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, (cons.cur_x + 1) * 8 + cons.off_x - 1, (cons.cur_y + 1) * 16 + cons.off_y - 1);
                //putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, COL8_008484, COL8_000000, " ", 1);
            }
            if (data & KEYSIG_BIT) {
                data &= ~KEYSIG_BIT;
                if (data == '\b') {
                    if (cons.cur_x > 1) {
                        cons_putchar(&cons, ' ', FALSE);
                        cons.cur_x--;
                    }
                } else if (data == '\n') {
                    putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, COL8_FFFFFF, COL8_000000, " ", 1);
                    cons_putchar(&cons, ' ', FALSE);
                    cmdline[cons.cur_x - 1] = '\0';
                    //putfonts8_sht(sheet, 20, 20, COL8_FF0000, COL8_008484, cmdline, strlen(cmdline));
                    //cons.cur_x = 0;
                    //cons.cur_y = cons_newline(cons.cur_y, sheet);
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);
                    cons_putchar(&cons, '>', TRUE);

                } else {
                    if (cons.cur_x + 1 < cons.width) {
                        //buf[0] = data;
                        //buf[1] = '\0';
                        cmdline[cons.cur_x - 1] = data;
                        //putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + off_y, COL8_FFFFFF, COL8_000000, buf, strlen(buf));
                        //cons.cur_x++;
                        cons_putchar(&cons, data, TRUE);
                    }
                }
            }
            if (cons.cur_c >= 0) putfonts8_sht(sheet, cons.cur_x * 8 + cons.off_x, cons.cur_y * 16 + cons.off_y, COL8_008484, cons.cur_c, " ", 1);
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
                putfonts8_sht(cons->sht, cons->cur_x * 8 + cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, " ", 1);
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
            putfonts8_sht(cons->sht, cons->cur_x * 8 + cons->off_x, cons->cur_y * 16 + cons->off_y, COL8_FFFFFF, COL8_000000, s, strlen(s));
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
    char buf[80];

    sprintf(buf, "cmd_app verbose: cmdline: %s\n", cmdline);
    cons_putstr0(cons, buf);

    do {
        if (!cmdline[0]) {
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "mem") || !strcmp(cmdline, "free")) {
            cmd_free(cons, memtotal);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "clear") || !strcmp(cmdline, "cls")) {
            cmd_clear(cons);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "ls") || !strcmp(cmdline, "dir")) {
            cmd_ls(cons);
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
            unsigned char word[16];
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
            unsigned char word[16];
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
            exit_success = !cmd_cat(cons, fat, cmdline);
            break;
        }
        if (!strcmp(cmdline, "debug")) {
            cmd_debug(cons);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "exit")) {
            cmd_exit(cons, isAcpiAvail);
            exit_success = TRUE;
            break;
        }
        if (!strcmp(cmdline, "test")) {
            cmd_test(cons);
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
            if (((unsigned char*)addr)[i] != word[i]) match = FALSE;
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

void cmd_exit(struct CONSOLE* cons, char mode) {
    cons_putstr0(cons, "Exiting...\n");
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
    cons_putstr0(cons, "Failed to exit.\n\n");
    return;
}

extern dword* SMI_CMD;
extern byte ACPI_ENABLE;
extern byte ACPI_DISABLE;
extern dword* PM1a_CNT;
extern dword* PM1b_CNT;
extern word SLP_TYPa;
extern word SLP_TYPb;
extern word SLP_EN;
extern word SCI_EN;
extern byte PM1_CNT_LEN;

void cmd_test(struct CONSOLE* cons) {
    char buf[80];
    sprintf(buf, "sizeof(struct RSDPtr): %d\n", sizeof(struct RSDPtr));
    wrstr(buf);
    uint* ptr = acpiCheckRSDPtr((uint*)0xe0000);
    sprintf(buf, "acpiCheckRSDPtr(0xe0000): %x\n", ptr);
    wrstr(buf);
    sprintf(buf, "SLP_TYPa: %x\n", SLP_TYPa);
    wrstr(buf);
    sprintf(buf, "SLP_TYPb: %x\n", SLP_TYPb);
    wrstr(buf);
    return;
}

#define SIZE_APPMEM (64 * 1024)

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
    finfo = file_search(name, (struct FILEINFO*)(ADDR_DISKIMG + 0x002600), 224);
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
            set_segmdesc(gdt + task->sel / 8 + 1000, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
            set_segmdesc(gdt + task->sel / 8 + 2000, segsiz - 1, (int)q, AR_DATA32_RW + 0x60);
            for (int i = 0; i < datsiz; ++i) q[esp + i] = p[dathrb + i];
            start_app(0x1b, task->sel + 1000 * 8, esp, task->sel + 2000 * 8, &(task->tss.esp0));

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
            sht = sheet_alloc(shtctl);
            sht->task = task;
            sht->flags |= SHEET_FLAGS_APP;
            sheet_setbuf(sht, (char*)ebx + ds_base, esi, edi, eax);
            make_window8((char*)ebx + ds_base, esi, edi, (char*)ecx + ds_base, 0);
            sheet_slide(sht, (shtctl->xsize - esi) / 2, (shtctl->ysize - edi) / 2);
            sheet_updown(sht, shtctl->top);  // on task_a
            reg[7] = (int)sht;     // removed when -O2
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
                }
                if (i >= KEYSIG_BIT) {  // keyboard signal
                    reg[7] = i & ~KEYSIG_BIT;
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
            timer_init((struct TIMER*)ebx, &task->fifo, eax + KEYSIG_BIT);
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