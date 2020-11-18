#pragma once

int inb(int port);
int inw(int port);
int inl(int port);
void outb(int value, int port);
void outw(int value, int port);
void outl(int value, int port);
void init_sleep();
unsigned int sleep(unsigned int milsec);
void wrstr(const char* str);