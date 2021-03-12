#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

int isalpha(int ch) {
    return isupper(ch) || islower(ch);
}
