#include "apilib.h"
#include "general.h"

#define eight(h)                            \
    {                                       \
        api_beep(h*10);                        \
        api_settimer(timer, 45);            \
        if (api_getkey(TRUE) != 128) break; \
    }

void HariMain(){
    int timer;
    int hz[] = {52325, 58733, 65926, 69846, 78399, 88000, 98777, 104650,
                104650, 98777, 83061, 78399, 69846, 62225, 58733, 52325};
    api_inittimer(timer = api_alloctimer(), 128);
    for (int i = 0; i < 1; i++){
        // from 20000 Hz to 20 Hz
        //b- r f f f+ - f r r r a r b- r
        for (int j = 0; j < sizeof(hz) / sizeof(int); ++j) eight(hz[j]);
    }
    api_beep(0);
    for (int p = 0; !p; p = api_getkey(FALSE)) {}
    api_end();
}
