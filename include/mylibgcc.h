#pragma once

#include "general.h"

typedef char* va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_copy(d, s) __builtin_va_copy(d, s)

size_t strlen(const char* s);
char* strcpy(char* restrict s1, const char* restrict s2);
char* strncpy(char* restrict s1, const char* restrict s2, size_t n);
void* memcpy(void* restrict s1, const void* restrict s2, size_t n);
char* strcat(char* restrict s1, const char* restrict s2);
char* strrev(char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* itoa(int value, char* str, int base);
char* utoa(unsigned int value, char* str, int base);
int atoi(const char* str);
int isupper(int ch);
int islower(int ch);
int isalpha(int ch);
int isdigit(int ch);
int isalnum(int ch);
int isxdigit(int ch);
int toupper(int ch);
int tolower(int ch);
int sprintf(char* restrict s, const char* restrict format, ...);
uint rand_xor32(void);
