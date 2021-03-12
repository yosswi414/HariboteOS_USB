#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

int isxdigit(int ch) {
    return isdigit(ch) || ('A' <= toupper(ch) && toupper(ch) <= 'F');
}
