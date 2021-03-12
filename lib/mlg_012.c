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

int islower(int ch) {
    if ('a' <= ch && ch <= 'z') return TRUE;
    return FALSE;
}

int isalpha(int ch) {
    return isupper(ch) || islower(ch);
}

int isdigit(int ch) {
    if ('0' <= ch && ch <= '9') return TRUE;
    return FALSE;
}

int isalnum(int ch) {
    return isalpha(ch) || isdigit(ch);
}

int isspace(int ch) {
    if (ch == ' ' || ch == '\f' || ch == '\n' || ch == '\r' || ch == '\t' || ch == '\v') return TRUE;
    return FALSE;
}

int isxdigit(int ch) {
    return isdigit(ch) || ('A' <= toupper(ch) && toupper(ch) <= 'F');
}

int toupper(int ch) {
    if (islower(ch)) ch -= 0x20;
    return ch;
}

int tolower(int ch) {
    if (isupper(ch)) ch += 0x20;
    return ch;
}
