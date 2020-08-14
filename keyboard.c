#include "asmfunc.h"
#include "device.h"
#include "fifo.h"
#include "interrupt.h"

void wait_KBC_sendready(void) {
    for (;;)
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) break;
    return;
}

void init_keyboard(void) {
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

// interrupt from PS/2 keyboard
struct FIFO8 keyfifo;

void inthandler21(int* esp) {
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61); // notify PIC that IRQ01 has been accepted
    data = io_in8(PORT_KEYDAT); // receive key code
    fifo8_put(&keyfifo, data);
    return;
}
