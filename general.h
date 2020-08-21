#ifndef _GENERAL_H_
#define _GENERAL_H_

// this header is included by every header
// (except by itself)

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

#endif