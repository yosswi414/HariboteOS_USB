#pragma once

unsigned char inb(unsigned short int port);
unsigned short int inw(unsigned short int port);
unsigned int inl(unsigned short int port);
void outb(unsigned char value, unsigned short int port);
void outw(unsigned short int value, unsigned short int port);
void outl(unsigned int value, unsigned short int port);
void init_sleep();
unsigned int sleep(unsigned int milsec);
void wrstr(const char* str);