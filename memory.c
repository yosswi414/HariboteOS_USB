#include "memory.h"
#include "asmfunc.h"
#include "mylibgcc.h"

uint memtest(uint start, uint end) {
    char flg486 = 0;
    uint eflg, cr0, ret;

    // check whether arch is 386 or 486 (or later)
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; // set AC-bit
    io_store_eflags(eflg);
    // in 386 even if we set AC-bit it would be reset automatically
    // which means if set successfully we are in 486 or later
    if (eflg & EFLAGS_AC_BIT) flg486 = 1;
    eflg &= ~EFLAGS_AC_BIT; // clear AC-bit
    io_store_eflags(eflg);

    if (flg486) {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; // disable caching
        store_cr0(cr0);
    }

    ret = memtest_sub(start, end);

    if (flg486) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; // enable caching
        store_cr0(cr0);
    }

    return ret;
}

/*

// [DEPRECATED]
// this function has been implemented in assembly

uint memtest_sub(uint start, uint end) {
    uint i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
    char fail = FALSE;
    for (i = start; i <= end; i += MEMCHECK_UNIT) {
        p = (uint*)(i + 0xffc);
        old = *p;
        *p = pat0;
        *p ^= 0xffffffff;
        if (*p != pat1) {
            *p = old;
            break;
        }
        *p ^= 0xffffffff;
        if (*p != pat0) {
            *p = old;
            break;
        }
        *p = old;
    }
    return i;
}
*/

void memman_init(struct MEMMAN* man) {
    man->frees = 0;
    man->maxfrees = 0;
    man->lostsize = 0;
    man->losts = 0;
    return;
}

uint memman_total(struct MEMMAN* man) {
    uint t = 0;
    for (uint i = 0; i < man->frees; ++i) t += man->free[i].size;
    return t;
}

uint memman_alloc(struct MEMMAN* man, uint size) {
    for (uint i = 0; i < man->frees; ++i) {
        if (man->free[i].size >= size) {
            uint a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {
                man->frees--;
                for (; i < man->frees; ++i) man->free[i] = man->free[i + 1];
            }
            return a;
        }
    }
    return 0;
}

int memman_free(struct MEMMAN* man, uint addr, uint size) {
    int i;
    for (i = 0; i < man->frees; ++i)
        if (man->free[i].addr > addr) break;

    if (i > 0 && man->free[i - 1].addr + man->free[i - 1].size == addr) {
        man->free[i - 1].size += size;
        if (i < man->frees && addr + size == man->free[i].addr) {
            man->free[i - 1].size += man->free[i].size;
            man->frees--;
            for (; i < man->frees; ++i) man->free[i] = man->free[i + 1];
        }
        return 0;
    }

    if (i < man->frees && addr + size == man->free[i].addr) {
        man->free[i].addr = addr;
        man->free[i].size += size;
        return 0;
    }

    if (man->frees < MEMMAN_FREES) {
        for (int j = man->frees; j > i; --j) man->free[j] = man->free[j - 1];
        ++man->frees;
        man->maxfrees = max(man->maxfrees, man->frees);
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }
    
    ++man->losts;
    man->lostsize += size;
    return -1;
}

uint memman_alloc_4k(struct MEMMAN* man, uint size) {
    return memman_alloc(man, (size + 0xfff) & 0xfffff000);
}

int memman_free_4k(struct MEMMAN* man, uint addr, uint size) {
    return memman_free(man, addr, (size + 0xfff) & 0xfffff000);
}
