#include "interrupt.h"

#include "asmfunc.h"

void init_pic(void) {
    // Interrupt Mask Register
    io_out8(PIC0_IMR, 0xff);  // deny all interrupt
    io_out8(PIC1_IMR, 0xff);  // deny all interrupt

    // Initial Control Word
    io_out8(PIC0_ICW1, 0x11);    // edge trigger mode
    io_out8(PIC0_ICW2, 0x20);    // receive IRQ0-7 by INT20-27
    io_out8(PIC0_ICW3, 1 << 2);  // PIC1(slave) is connected at IRQ2
    io_out8(PIC0_ICW4, 0x01);    // non-buffer mode

    io_out8(PIC1_ICW1, 0x11);  // edge trigger mode
    io_out8(PIC1_ICW2, 0x28);  // receive IRQ8-15 by INT28-2f
    io_out8(PIC1_ICW3, 2);     // PIC0(master) is connected at IRQ2
    io_out8(PIC1_ICW4, 0x01);  // non-buffer mode

    io_out8(PIC0_IMR, 0xfb);  // any interrupt but by PIC1 will be denied
    io_out8(PIC1_IMR, 0xff);  // any interrupt will be denied

    return;
}

// interrupt from PIC0 (defective)
// with a certain chipset, this interrupt may be invoked once at PIC initialization
// this is caused by electronic noise, so nothing has to be done.
void inthandler27(int* esp) {
    io_out8(PIC0_OCW2, 0x67);  // notify PIC that IRQ7 is received
    return;
}