#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

int tolower(int ch) {
    if (isupper(ch)) ch += 0x20;
    return ch;
}
