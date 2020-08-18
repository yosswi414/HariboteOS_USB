#ifndef _MYLIBGCC_H_
#define _MYLIBGCC_H_

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