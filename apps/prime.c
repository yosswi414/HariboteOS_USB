#include "apilib.h"
#include "mylibgcc.h"

// char isPrime(int p){
//     if (p < 2) return FALSE;
//     for (int q = 2; q * q <= p; ++q)
//         if (p % q == 0) return FALSE;
//     return TRUE;
// }
#define N 100000
void HariMain() {
    // api_initmalloc();
    // char *isPrime = api_malloc(N);
    char isPrime[N];
    char str[8];
    for (int i = 0; i < N; ++i) isPrime[i] = 0x55;
    isPrime[0] = isPrime[1] = '@';
    for (int p = 2; p < N; ++p) {
        if (!isPrime[p]) continue;
        for (int k = 2; k * p < N; ++k) isPrime[k * p] = 0x00;
    }
    for (int p = 2; p < N; ++p) {
        if (isPrime[p]) {
            // if (isPrime(p)) {
            sprintf(str, "%d ", p);
            api_putstr0(str);
        }
    }
    api_putstr0("\n");
    sprintf(str, "\nisPrime: 0x%x\n", isPrime);
    api_putstr0(str);
    sprintf(str, "head: 0x%x\n", *((int*)isPrime));
    api_putstr0(str);
    api_end();
}