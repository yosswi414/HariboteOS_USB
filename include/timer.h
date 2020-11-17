#pragma once

#include "fifo.h"
#include "general.h"

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
    struct TIMER* next;
    uint timeout, flags;
    struct FIFO32* fifo;
    int data;
};

struct TIMERCTL {
    uint count, next;
    struct TIMER* t0;
    struct TIMER timers0[MAX_TIMER];
};

void init_pit(void);
void inthandler20(int* esp);
struct TIMER* timer_alloc(void);
void timer_free(struct TIMER* timer);
void timer_init(struct TIMER* timer, struct FIFO32* fifo, int data);
void timer_settime(struct TIMER* timer, uint timeout);
void timer_adjust();
