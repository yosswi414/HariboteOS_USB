#ifndef _FIFO_H_
#define _FIFO_H_

struct FIFO32 {
    int* buf;
    int p, q, size, free, flags;
    struct TASK* task;
};

#define FIFO_FLAGS_NORMAL 0
#define FIFO_FLAGS_OVERRUN 1

void fifo32_init(struct FIFO32* fifo, int size, int* buf, struct TASK *task);
int fifo32_put(struct FIFO32* fifo, int data);
int fifo32_get(struct FIFO32* fifo);
int fifo32_status(struct FIFO32* fifo);

#endif