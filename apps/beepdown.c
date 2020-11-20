#include "api.h"
#include "general.h"

#define eight(h)                            \
    {                                       \
        api_beep(h*10);                        \
        api_settimer(timer, 45);            \
        if (api_getkey(TRUE) != 128) break; \
    }

void HariMain(){
    int timer;
    api_inittimer(timer = api_alloctimer(), 128);
    for (int i = 0; i < 1; i++){
        // from 20000 Hz to 20 Hz
        //b- r f f f+ - f r r r a r b- r
        eight(52325);
        eight(58733);
        eight(65926);
        eight(69846);
        eight(78399);
        eight(88000);
        eight(98777);
        eight(104650);
        eight(104650);
        eight(98777);
        eight(83061);
        eight(78399);
        eight(69846);
        eight(62225);
        eight(58733);
        eight(52325);
    }
    api_beep(0);
    for (int p = 0; !p; p = api_getkey(FALSE)) {}
    api_end();
}
