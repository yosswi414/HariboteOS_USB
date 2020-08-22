#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "fifo.h"

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32* fifo, int data0);
void enable_mouse(struct FIFO32* fifo, int data0, struct MOUSE_DEC* mdec);
int mouse_decode(struct MOUSE_DEC* m_dec, unsigned char dat);

void inthandler21(int* esp);
void inthandler2c(int* esp);

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

#define KEYSIG_BIT 0x100
#define MOUSESIG_BIT 0x200

#define MAX_MOUSEQUE 32

#endif