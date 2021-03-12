#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

int isupper(int ch) {
    if ('A' <= ch && ch <= 'Z') return TRUE;
    return FALSE;
}
