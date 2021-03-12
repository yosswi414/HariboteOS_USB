#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

int islower(int ch) {
    if ('a' <= ch && ch <= 'z') return TRUE;
    return FALSE;
}
