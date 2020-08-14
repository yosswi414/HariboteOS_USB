#ifndef _MYLIBGCC_H_
#define _MYLIBGCC_H_

int strlen(const char* s);
char* strcpy(char* restrict s1, const char* restrict s2);
char* strcat(char* s1, const char* s2);
char* strrev(char* s);
int abs(int x);
char* itoa(int value, char* str, int base);
char* utoa(unsigned int value, char* str, int base);
#define abs(x) ((x) < 0 ? (-(x)) : (x))
#define max(a, b) ((a) < (b) ? (b) : (a))
#define min(a, b) ((a) < (b) ? (a) : (b))
int toupper(int ch);
int tolower(int ch);
int sprintf(char* restrict s, const char* restrict format, ...);

#endif