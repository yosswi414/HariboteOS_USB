#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

int isdigit(int ch) {
    if ('0' <= ch && ch <= '9') return TRUE;
    return FALSE;
}
