#include "api.h"
#include "general.h"

void HariMain(){
    int timer;
    api_inittimer(timer = api_alloctimer(), 128);
    for (int i = 1000 * 1000; i >= 20 * 1000; i -= i / 1000){
        // from 20000 Hz to 20 Hz
        api_beep(i);
        api_settimer(timer, 1); // 10 ms
        if (api_getkey(TRUE) != 128) break;
    }
    api_beep(0);
    api_end();
}