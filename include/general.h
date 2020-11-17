#pragma once

#ifdef using
#undef using
#endif

#define TRUE (1)
#define FALSE (0)
#define NULL ((void *)0)
#define EOF (-1)

#define abs(x) ((x) < 0 ? (-(x)) : (x))
#define max(a, b) ((a) < (b) ? (b) : (a))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define clamp(x, l, r) min(max(x, l), r)

typedef unsigned int uint;
typedef long unsigned int size_t;

typedef unsigned int dword;
typedef unsigned short word;
typedef unsigned char byte;