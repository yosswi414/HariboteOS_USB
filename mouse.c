#include "device.h"
#include "asmfunc.h"
#include "fifo.h"
#include "interrupt.h"

void enable_mouse(struct MOUSE_DEC* mdec) {
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // if success, ACK (0xfa) will be sent
    mdec->phase = 0;
    return;
}

int mouse_decode(struct MOUSE_DEC* mdec, unsigned char dat) {
    switch (mdec->phase) {
    case 0:
        if (dat == 0xfa) mdec->phase = 1;
        break;
    case 1:
        mdec->buf[0] = dat;
        if((dat&0x08)&&!(dat&0xc0))mdec->phase = 2;
        break;
    case 2:
        mdec->buf[1] = dat;
        mdec->phase = 3;
        break;
    case 3:
        mdec->buf[2] = dat;
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if (mdec->buf[0] & 0x10) mdec->x |= 0xffffff00;
        if (mdec->buf[0] & 0x20) mdec->y |= 0xffffff00;
        mdec->y = -mdec->y;
        return 1;
    default:
        return -1;
    }
    return 0;
}

// interrupt from PS/2 mouse
struct FIFO8 mousefifo;

void inthandler2c(int* esp) {
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64); // notify PIC that IRQ12 has been accepted
    io_out8(PIC0_OCW2, 0x62); // notify PIC that IRQ02 has been accepted
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
    return;
}