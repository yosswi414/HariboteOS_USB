#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

char* strrev(char* s) {
    int len = strlen(s);
    if (len >= STR_LIM) return 0;
    char t;
    for (int i = 0; i < len / 2; ++i) {
        t = s[i];
        s[i] = s[len - i - 1];
        s[len - i - 1] = t;
    }
    return s;
}
