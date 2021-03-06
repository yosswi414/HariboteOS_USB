#pragma once

#include "general.h"

extern char ascii[0x1000];

#define ADDR_BOOTINFO 0x00000ff0
#define ADDR_DISKIMG 0x0100000

struct BOOTINFO {
    unsigned char cyls;   // how far the data on the disk is loaded to boot sector
    unsigned char leds;   // the state of keyboard LED when boot
    unsigned char vmode;  // video mode: decide n-bit color
    unsigned char reserve;
    short scrnx, scrny;
    unsigned char* vram;
};

void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
int io_in16(int port);
int io_in32(int port);
void io_out8(int port, int data);
void io_out16(int port, int data);
void io_out32(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler00(void);
void asm_inthandler06(void);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
int load_cr0(void);
void store_cr0(int cr0);
uint memtest_sub(uint start, uint end);
void load_tr(int tr);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_cons_putchar();
void asm_hrb_api();
void start_app(int eip, int cs, int esp, int ds, int* tss_esp0);
void asm_end_app(int eip);

void asm_exit();
void asm_exit_int();
int asm_apm_instl_chk();