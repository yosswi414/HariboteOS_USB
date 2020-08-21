#ifndef _MYLIBGCC_H_
#define _MYLIBGCC_H_

typedef char* va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_copy(d, s) __builtin_va_copy(d, s)

int strlen(const char* s);
char* strcpy(char* restrict s1, const char* restrict s2);
char* strcat(char* s1, const char* s2);
char* strrev(char* s);
char* itoa(int value, char* str, int base);
char* utoa(unsigned int value, char* str, int base);
int toupper(int ch);
int tolower(int ch);
int sprintf(char* restrict s, const char* restrict format, ...);

#endif