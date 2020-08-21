#ifndef _TIMER_H_
#define _TIMER_H_

#include "general.h"
#include "fifo.h"

// PIT; Programmable Interval Timer

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

// CLK / PERIOD [Hz]
#define TIMER_PERIOD ((unsigned short)0x2e9c)

#define MAX_TIMER 500

#define TIMER_FLAGS_EMPTY 0
#define TIMER_FLAGS_ALLOC 1
#define TIMER_FLAGS_USING 2

struct TIMER {
    uint timeout, flags;
    struct FIFO8* fifo;
    unsigned char data;
};

struct TIMERCTL {
    uint count, next, using;
    struct TIMER* timers[MAX_TIMER];
    struct TIMER timers0[MAX_TIMER];
};

void init_pit(void);
void inthandler20(int* esp);
struct TIMER* timer_alloc(void);
void timer_free(struct TIMER* timer);
void timer_init(struct TIMER* timer, struct FIFO8* fifo, unsigned char data);
void timer_settime(struct TIMER* timer, uint timeout);

#endif