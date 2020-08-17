#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "general.h"

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000
#define MEMCHECK_UNIT 0x100000
#define MEMMAN_FREES 4090
#define MEMMAN_ADDR 0x003c0000

struct FREEINFO {
    uint addr, size;
};

struct MEMMAN {
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

uint memtest(uint start, uint end);
// [DEPRECATED]
// this function has been implemented in assembly
//uint memtest_sub(uint start, uint end);

void memman_init(struct MEMMAN* man);
uint memman_total(struct MEMMAN* man);
uint memman_alloc(struct MEMMAN* man, uint size);
int memman_free(struct MEMMAN* man, uint addr, uint size);

#endif