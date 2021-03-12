#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

int isalnum(int ch) {
    return isalpha(ch) || isdigit(ch);
}
