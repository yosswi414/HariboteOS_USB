#pragma once

#include "fifo.h"

static char keytable[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '@', '[', 0,
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', ':', 0, 0, ']',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '\\', 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '\\', 0, 0};

static char keytable_shift[] = {
    0, 0, '!', '\"', '#', '$', '%', '&', '\'', '(', ')', 0, '=', '~', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '`', '{', 0,
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '+', '*', 0, 0, '}',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0, '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '|', 0, 0};

static char* npktable[] = {
    "Printable",
    "Esc",
    "Backspace",
    "Tab",
    "Enter full",
    "LCtrl",
    "Han/Zen",
    "LShift",
    "RShift",
    "LAlt",
    "CapsLock",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "NumLock",
    "ScrollLock",
    "SysReq",
    "F11",
    "F12",
    "Hiragana",
    "Convert",
    "Unconvert"};

struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32* fifo, int data0);
void enable_mouse(struct FIFO32* fifo, int data0, struct MOUSE_DEC* mdec);
int mouse_decode(struct MOUSE_DEC* m_dec, unsigned char dat);
char keycode_tochar(int code, char shift);
char* keycode_toname(int code);

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
