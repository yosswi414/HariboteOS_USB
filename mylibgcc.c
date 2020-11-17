#include "mylibgcc.h"

#include "asmfunc.h"
#include "general.h"
#include "timer.h"
#include "fifo.h"
#include "console.h"

#define BUF_SIZ (256)
#define STR_LIM (1024)

size_t strlen(const char* s) {
    int len = 0;
    while (s[len] != '\0' && len <= STR_LIM) ++len;
    if (STR_LIM < len) return STR_LIM;
    return len;
}

char* strcpy(char* restrict s1, const char* restrict s2) {
    int pos = 0;
    char ch;
    do {
        ch = s2[pos];
        s1[pos++] = ch;
        if (ch == '\0') break;
    } while (pos < STR_LIM);
    return s1;
}

char* strncpy(char* restrict s1, const char* restrict s2, size_t n) {
    int pos = 0;
    char ch;
    do {
        ch = s2[pos];
        s1[pos++] = ch;
        if (ch == '\0') break;
    } while (pos < n);
    return s1;
}

void* memcpy(void* restrict s1, const void* restrict s2, size_t n) {
    unsigned char* dst = s1;
    const unsigned char* src = s2;
    while (n--) *(dst++) = *(src++);
    return s1;
}

char* strcat(char* restrict s1, const char* restrict s2) {
    char* dst = s1;
    const char* src = s2;
    while (*dst) dst++;
    while (*(dst++) = *(src++)) {}
    return s1;
}

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

// positive if s1 > s2, negative if s1 < s2, zero if equal
int strcmp(const char* s1, const char* s2) {
    for (int i = 0;; ++i) {
        if (!s1[i] && !s2[i]) return 0;
        if ((s1[i] && !s2[i]) || s1[i] > s2[i]) return 1;
        if ((!s1[i] && s2[i]) || s1[i] < s2[i]) return -1;
    }
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (int i = 0; i < n; ++i) {
        if ((s1[i] && !s2[i]) || s1[i] > s2[i]) return 1;
        if ((!s1[i] && s2[i]) || s1[i] < s2[i]) return -1;
    }
    return 0;
}

// put affix like "0x-" in hex or "-b" in bin
// in general no affix would be attached
#define ENABLE_AFFIX FALSE
char* itoa(int value, char* str, int base) {
    int is_neg = FALSE;
    int pos = 0;
    if (base == 10 && value < 0) {
        is_neg = TRUE;
    }
    if (base == 2 && ENABLE_AFFIX) str[pos++] = 'b';
    if (value == 0) str[pos++] = '0';
    while (value != 0) {
        int digit = abs(value % base);
        str[pos++] = (digit > 9 ? digit - 10 + 'a' : digit + '0');
        value /= base;
    }
    if (ENABLE_AFFIX) {
        if (base == 16) str[pos++] = 'x';
        if (base % 8 == 0) str[pos++] = '0';
    }
    if (is_neg) str[pos++] = '-';
    str[pos] = '\0';
    return strrev(str);
}

char* utoa(uint value, char* str, int base) {
    int pos = 0;
    if (base == 2 && ENABLE_AFFIX) str[pos++] = 'b';
    if (value == 0) str[pos++] = '0';
    while (value != 0) {
        int digit = abs(value % base);
        str[pos++] = (digit > 9 ? digit - 10 + 'a' : digit + '0');
        value /= base;
    }
    if (ENABLE_AFFIX) {
        if (base == 16) str[pos++] = 'x';
        if (base % 8 == 0) str[pos++] = '0';
    }
    str[pos] = '\0';
    return strrev(str);
}

int atoi(const char* str) {
    int len = strlen(str);
    int base = 10;
    int u, r = 0, i = 0;
    char neg = FALSE;
    if (str[0] == '-') neg = TRUE, i++;
    if (str[0] == '0') {
        base = 8, i++;
        if (tolower(str[1]) == 'x') base = 16, i++;
    } else if (tolower(str[len - 1]) == 'b') {
        base = 2;
        len--;
    }
    for (; i < len && isxdigit(str[i]); ++i) {
        if ('a' <= tolower(str[i]) && tolower(str[i]) <= 'f')
            u = tolower(str[i]) - 'a' + 10;
        else
            u = str[i] - '0';
        r = r * base + (neg ? -u : u);
    }
    return r;
}

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
                    if (len + strlen(p) + 1 > BUF_SIZ) is_valid = FALSE;
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

uint rand_xor32(void) {
    static uint y = 2463534242u;
    y = y ^ (y << 13);
    y = y ^ (y >> 17);
    return y = y ^ (y << 5);
}


int memcmp(const void* restrict buf1, const void* restrict buf2, size_t n) {
    const unsigned char* p1 = buf1;
    const unsigned char* p2 = buf2;
    for (size_t i = 0; i < n; ++i, ++p1, ++p2)
        if (*p1 != *p2) return 1 - 2 * (*p1 < *p2);
    return 0;
}