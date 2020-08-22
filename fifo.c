#include "fifo.h"

void fifo32_init(struct FIFO32* fifo, int size, int* buf) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = FIFO_FLAGS_NORMAL;
    fifo->p = 0;
    fifo->q = 0;
}

int fifo32_put(struct FIFO32* fifo, int data) {
    if (fifo->free == 0) {
        fifo->flags |= FIFO_FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p++] = data;
    fifo->p %= fifo->size;
    fifo->free--;
    return 0;
}

int fifo32_get(struct FIFO32* fifo) {
    int data;
    if (fifo->free == fifo->size) return -1;
    data = fifo->buf[fifo->q++];
    fifo->q %= fifo->size;
    fifo->free++;
    return data;
}

int fifo32_status(struct FIFO32* fifo) {
    return fifo->size - fifo->free;
}