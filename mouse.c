#include "asmfunc.h"
#include "device.h"
#include "fifo.h"
#include "interrupt.h"

struct FIFO32* mouse_fifo;
int mouse_offset, mouse_queue;

void enable_mouse(struct FIFO32* fifo, int data0, struct MOUSE_DEC* mdec) {
    mouse_fifo = fifo;
    mouse_offset = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // if success, ACK (0xfa) will be sent
    mdec->phase = 0;
    mouse_queue = 0;
    return;
}

int mouse_decode(struct MOUSE_DEC* mdec, unsigned char dat) {
    mouse_queue--;
    switch (mdec->phase) {
    case 0:
        if (dat == 0xfa) mdec->phase = 1;
        break;
    case 1:
        mdec->buf[0] = dat;
        if ((dat & 0x08) && !(dat & 0xc0)) mdec->phase = 2;
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

void inthandler2c(int* esp) {
    int data;
    io_out8(PIC1_OCW2, 0x64); // notify PIC that IRQ12 has been accepted
    io_out8(PIC0_OCW2, 0x62); // notify PIC that IRQ02 has been accepted
    data = io_in8(PORT_KEYDAT);
    if (mouse_queue < MAX_MOUSEQUE * 3) {
        fifo32_put(mouse_fifo, data + mouse_offset);
        mouse_queue++;
    }
    return;
}