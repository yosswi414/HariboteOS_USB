#include "timer.h"

#include "asmfunc.h"
#include "fifo.h"
#include "general.h"
#include "interrupt.h"
#include "mtask.h"

extern int dbg_val[4];
struct TIMERCTL timerctl;
char ENABLE_TIMECNT = FALSE;

// change interrupt period
// *0x43 <- 0x34
// *0x40 <- lower 8 bit of int. period
// *0x40 <- upper 8 bit of int. period
// if int. period is set 0, it will behave as set to 65536.
void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, (unsigned char)(TIMER_PERIOD & 0x00ff));
    io_out8(PIT_CNT0, (unsigned char)((TIMER_PERIOD & 0xff00) >> 8));
    for (int i = 0; i < MAX_TIMER; ++i) timerctl.timers0[i].flags = TIMER_FLAGS_EMPTY;
    struct TIMER* t = timer_alloc();
    t->timeout = timerctl.next = 0xffffffff;
    t->flags = TIMER_FLAGS_USING;
    t->next = NULL;
    timerctl.t0 = t;
    timerctl.count = 0;
    return;
}

// from mtask.c
extern struct TIMER* task_timer;

void inthandler20(int* esp) {
    char ts = 0;

    io_out8(PIC0_OCW2, 0x60);  // ACK
    if (!ENABLE_TIMECNT) return;
    timerctl.count++;
    if (timerctl.next > timerctl.count) return;
    struct TIMER* timer;
    for (timer = timerctl.t0; timer->timeout <= timerctl.count; timer = timer->next) {
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != task_timer)
            fifo32_put(timer->fifo, timer->data);
        else
            ts = 1;
    }
    timerctl.t0 = timer;
    timerctl.next = timerctl.t0->timeout;
    if (ts) task_switch();
    return;
}

struct TIMER* timer_alloc(void) {
    for (int i = 0; i < MAX_TIMER; ++i) {
        if (timerctl.timers0[i].flags == TIMER_FLAGS_EMPTY) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return NULL;
}

void timer_free(struct TIMER* timer) {
    timer->flags = TIMER_FLAGS_EMPTY;
    return;
}

void timer_init(struct TIMER* timer, struct FIFO32* fifo, int data) {
    if (fifo) timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER* timer, uint timeout) {
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    int e = io_load_eflags();
    io_cli();
    struct TIMER* t = timerctl.t0;
    if (timer->timeout <= t->timeout) {
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next = timer->timeout;
        io_store_eflags(e);
        return;
    }
    for (struct TIMER* s;;) {
        s = t;
        t = t->next;
        if (timer->timeout <= t->timeout) {
            s->next = timer;
            timer->next = t;
            io_store_eflags(e);
            return;
        }
    }
    io_store_eflags(e);
    return;
}

void timer_adjust() {
    int t0 = timerctl.count;
    io_cli();
    timerctl.count -= t0;
    for (int i = 0; i < MAX_TIMER; ++i) {
        if (timerctl.timers0[i].timeout != 0xffffffff) timerctl.timers0[i].timeout -= t0;
    }
    io_sti();
}