#include "sysfunc.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"
#include "asmfunc.h"

int inb(int port) { return io_in8(port); }
int inw(int port) { return io_in16(port); }
int inl(int port) { return io_in32(port); }
void outb(int value, int port) { io_out8(port, value); }
void outw(int value, int port) { io_out16(port, value); }
void outl(int value, int port) { io_out32(port, value); }

struct TIMER* slp_timer;
struct FIFO32 slp_fifo;
int slp_fifo_buf[32];
void init_sleep() {
    if (slp_timer) return;
    fifo32_init(&slp_fifo, sizeof(slp_fifo_buf) / sizeof(int), slp_fifo_buf, NULL);
    slp_timer = timer_alloc();
    timer_init(slp_timer, &slp_fifo, 63);
}

// min time unit: 10 milliseconds
unsigned int sleep(uint milsec) {
    uint t = milsec / 10 + (milsec % 10 >= 5);
    if (!slp_timer) init_sleep();
    timer_settime(slp_timer, t);
    //TASK* task = task_now();
    for (;;) {
        if (!fifo32_status(&slp_fifo)){
            //task_sleep(((struct CONSOLE*)ADDR_CONSOLE)->sht->task);
            //task_sleep(task);
            io_hlt();
        } else {
            //task_run(task, -1, 0);
            return fifo32_get(&slp_fifo);
        }
    }
}

void wrstr(const char* str) {
    struct TASK* task = task_now();
    struct CONSOLE* cons = task->cons;
    cons_putstr0(cons, str);
}