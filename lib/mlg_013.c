#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

// available format: %c, %d, %u, %o, %X, %x, %b
int sprintf(char* restrict s, const char* restrict format, ...) {
    va_list ap;
    va_start(ap, format);
    int len = 0;
    char buf[BUF_SIZ];
    int is_valid = TRUE;

    for (char* ptr = (char*)format; is_valid && *ptr != '\0'; ++ptr) {
        if (*ptr != '%') {
            buf[len++] = *ptr;
            buf[len] = '\0';
        } else {
            char str[BUF_SIZ], filbuf[BUF_SIZ];
            char* p;
            //int fill = FALSE;
            int upper = FALSE;
            char fill_ch = ' ';
            int mfw = 0;   // minimum field width
            int prec = 0;  // precision
            str[0] = filbuf[0] = '\0';
            ++ptr;

            if (*ptr == '0') {
                fill_ch = '0';
                ++ptr;
            } else if (*ptr == ' ') {
                fill_ch = ' ';
                ++ptr;
            }

            while ('0' <= *ptr && *ptr <= '9') {
                mfw *= 10;
                mfw += *(ptr++) - '0';
            }
            if (*ptr == '.') {
                ++ptr;
                while ('0' <= *ptr && *ptr <= '9') {
                    prec *= 10;
                    prec += *(ptr++) - '0';
                }
            }
            switch (*ptr) {
                case 'c':
                    str[0] = va_arg(ap, int);  // warning: 'char' is promoted to 'int' when passed through '...'
                    str[1] = '\0';
                    break;
                case 's':
                    p = va_arg(ap, char*);
                    if (len + strlen(p) + 1 > BUF_SIZ)
                        is_valid = FALSE;
                    else
                        strcpy(str, p);
                    break;
                case 'd':
                    itoa(va_arg(ap, int), str, 10);
                    break;
                case 'u':
                    utoa(va_arg(ap, uint), str, 10);
                    break;
                case 'o':
                    utoa(va_arg(ap, uint), str, 8);
                    break;
                case 'X':
                    upper = TRUE;
                case 'x':
                    utoa(va_arg(ap, uint), str, 16);
                    break;
                case 'b':
                    utoa(va_arg(ap, uint), str, 2);
                    break;
                default:
                    is_valid = FALSE;
            }
            if (len + strlen(str) + 1 > BUF_SIZ) is_valid = FALSE;
            if (is_valid) {
                int nfil = max(mfw - strlen(str), 0);
                if (nfil + 1 > BUF_SIZ)
                    is_valid = FALSE;
                else {
                    for (int i = 0; i < nfil; ++i) filbuf[i] = fill_ch;
                    filbuf[nfil] = '\0';
                    if (fill_ch == '0' && str[0] == '-') {
                        str[0] = '0';
                        filbuf[0] = '-';
                    }
                    if (upper)
                        for (int i = 0; i < strlen(str); ++i) str[i] = toupper(str[i]);
                    buf[len] = '\0';
                    len = strlen(strcat(buf, strcat(filbuf, str)));
                }
            }
        }
        if (len + 1 > BUF_SIZ) is_valid = FALSE;
        if (!is_valid) break;
    }
    if (!is_valid) {
        strcpy(s, "sprintf(): failed to convert.");  //return -1;
        len = EOF;
    } else {
        strcpy(s, buf);
    }
    va_end(ap);
    return len;
}
