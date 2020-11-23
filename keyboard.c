#include "asmfunc.h"
#include "device.h"
#include "fifo.h"
#include "interrupt.h"
#include "mylibgcc.h"

void wait_KBC_sendready(void) {
    while (io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) {}
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
    io_out8(PIC0_OCW2, 0x61);    // notify PIC that IRQ01 has been accepted
    data = io_in8(PORT_KEYDAT);  // receive key code
    fifo32_put(key_fifo, data | key_offset);
    return;
}

char keycode_tochar(int code, char shift) {
    return shift ? toupper(keytable_shift[code]) : keytable[code];
}

char* keycode_toname(int code) {
    int pos = 0;
    switch (code) {
    case 0x7b: ++pos; // Unconvert
    case 0x79: ++pos; // Convert
    case 0x70: ++pos; // Hiragana
    case 0x58: ++pos; // F12
    case 0x57: ++pos; // F11
    case 0x54: ++pos; // SysReq
    case 0x46: ++pos; // ScrollLock
    case 0x45: ++pos; // NumLock
    case 0x44: ++pos; // F10
    case 0x43: ++pos; // F9
    case 0x42: ++pos; // F8
    case 0x41: ++pos; // F7
    case 0x40: ++pos; // F6
    case 0x3f: ++pos; // F5
    case 0x3e: ++pos; // F4
    case 0x3d: ++pos; // F3
    case 0x3c: ++pos; // F2
    case 0x3b: ++pos; // F1
    case 0x3a: ++pos; // CapsLock
    case 0x38: ++pos; // LAlt
    case 0x36: ++pos; // RShift
    case 0x2a: ++pos; // LShift
    case 0x29: ++pos; // Han/Zen
    case 0x1d: ++pos; // LCtrl
    case 0x1c: ++pos; // Enter full
    case 0x0f: ++pos; // Tab
    case 0x0e: ++pos; // Backspace
    case 0x01: ++pos; // Esc
    }
    return npktable[pos];
}