#ifndef _ASMFUNC_H_
#define _ASMFUNC_H_

extern char ascii[0x1000];

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
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

struct BOOTINFO {
    unsigned char cyls; // how far the data on the disk is loaded to boot sector
    unsigned char leds; // the state of keyboard LED when boot
    unsigned char vmode; // video mode: decide n-bit color
    unsigned char reserve;
    short scrnx, scrny;
    unsigned char* vram;
};

#define ADDR_BOOTINFO 0x0ff0

#endif