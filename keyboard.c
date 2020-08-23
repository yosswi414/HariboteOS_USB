#include "asmfunc.h"
#include "device.h"
#include "fifo.h"
#include "interrupt.h"
#include "mylibgcc.h"

void wait_KBC_sendready(void) {
    while (io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) { }
    return;
}

struct FIFO32* key_fifo;
int key_offset;

void init_keyboard(struct FIFO32* fifo, int data0) {
    key_fifo = fifo;
    key_offset = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

// interrupt from PS/2 keyboard

void inthandler21(int* esp) {
    int data;
    io_out8(PIC0_OCW2, 0x61); // notify PIC that IRQ01 has been accepted
    data = io_in8(PORT_KEYDAT); // receive key code
    fifo32_put(key_fifo, data + key_offset);
    return;
}

char keycode_tochar(int code, char shift) {
    return shift ? toupper(keytable_shift[code]) : keytable[code];
}

char* keycode_toname(int code) {
    int pos = 0;
    switch (code) {
    case 0x7b: ++pos;
    case 0x79: ++pos;
    case 0x70: ++pos;
    case 0x58: ++pos;
    case 0x57: ++pos;
    case 0x54: ++pos;
    case 0x46: ++pos;
    case 0x45: ++pos;
    case 0x44: ++pos;
    case 0x43: ++pos;
    case 0x42: ++pos;
    case 0x41: ++pos;
    case 0x40: ++pos;
    case 0x3f: ++pos;
    case 0x3e: ++pos;
    case 0x3d: ++pos;
    case 0x3c: ++pos;
    case 0x3b: ++pos;
    case 0x3a: ++pos;
    case 0x38: ++pos;
    case 0x36: ++pos;
    case 0x2a: ++pos;
    case 0x29: ++pos;
    case 0x1d: ++pos;
    case 0x1c: ++pos;
    case 0x0f: ++pos;
    case 0x0e: ++pos;
    case 0x01: ++pos;
    }
    return npktable[pos];
}