#include "sysfunc.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"
#include "asmfunc.h"

unsigned char inb(unsigned short int port) { return io_in8(port); }
unsigned short int inw(unsigned short int port) { return io_in16(port); }
unsigned int inl(unsigned short int port) { return io_in32(port); }
void outb(unsigned char value, unsigned short int port) { io_out8(port, value); }
void outw(unsigned short int value, unsigned short int port) { io_out16(port, value); }
void outl(unsigned int value, unsigned short int port) { io_out32(port, value); }

struct TIMER* slp_timer;
struct FIFO32 slp_fifo;
int slp_fifo_buf[32];
void init_sleep() {
    if (slp_timer) return;
    fifo32_init(&slp_fifo, sizeof(slp_fifo_buf) / sizeof(int), slp_fifo_buf, NULL);
    slp_timer = timer_alloc();
}

// min time unit: 10 milliseconds
unsigned int sleep(uint milsec) {
    uint t = milsec / 10 + (milsec % 10 >= 5);
    if (!slp_timer) init_sleep();
    timer_settime(slp_timer, t);
    for (;;) {
        io_cli();
        if (!fifo32_status(&slp_fifo))
            io_stihlt();
        else
            return 0;
    }
}

void wrstr(const char* str) {
    struct CONSOLE* cons = (struct CONSOLE*)*((int*)ADDR_CONSOLE);
    cons_putstr0(cons, str);
}