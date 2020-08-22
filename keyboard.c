#include "asmfunc.h"
#include "device.h"
#include "fifo.h"
#include "interrupt.h"

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
