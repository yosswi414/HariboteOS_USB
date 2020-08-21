#include "timer.h"
#include "asmfunc.h"
#include "fifo.h"
#include "general.h"
#include "interrupt.h"

extern char ENABLE_TIMECNT;
extern int dbg_val[4];
struct TIMERCTL timerctl;

void init_pit(void) {
    // change interrupt period
    // *0x43 <- 0x34
    // *0x40 <- lower 8 bit of int. period
    // *0x40 <- upper 8 bit of int. period
    //    if int. period is set 0, it will
    // be regarded as 65536.
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, (unsigned char)(TIMER_PERIOD & 0x00ff));
    io_out8(PIT_CNT0, (unsigned char)((TIMER_PERIOD & 0xff00) >> 8));
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    timerctl.using = 0;
    for (int i = 0; i < MAX_TIMER; ++i) timerctl.timers0[i].flags = TIMER_FLAGS_EMPTY;
    return;
}

void inthandler20(int* esp) {
    io_out8(PIC0_OCW2, 0x60); // ACK
    if (ENABLE_TIMECNT) {
        timerctl.count++;
        if (timerctl.next > timerctl.count) return;
        timerctl.next = 0xffffffff;
        int i;
        for (i = 0; i < timerctl.using; ++i) {
            if (timerctl.timers[i]->timeout > timerctl.count) break;
            timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
            fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
        }
        timerctl.using -= i;
        for (int j = 0; j < timerctl.using; ++j) timerctl.timers[j] = timerctl.timers[i + j];
        if (timerctl.using > 0)
            timerctl.next = timerctl.timers[0]->timeout;
        else
            timerctl.next = 0xffffffff;
    }
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

void timer_init(struct TIMER* timer, struct FIFO8* fifo, unsigned char data) {
    if (fifo) timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER* timer, uint timeout) {
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    int e = io_load_eflags();
    io_cli();
    int i = 0;
    while (i < timerctl.using && timerctl.timers[i]->timeout < timer->timeout) ++i;
    for (int j = timerctl.using; j > i; --j) timerctl.timers[j] = timerctl.timers[j - 1];
    timerctl.using ++;
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(e);
    return;
}