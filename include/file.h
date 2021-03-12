#pragma once

#define MAX_FILES 8

struct FILEINFO {
    char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date, clustno;
    unsigned int size;
};

struct FILEHANDLE{
    char* buf;
    int size;
    int pos;
};

void file_readfat(int* fat, unsigned char* img);
void file_loadfile(int clustno, int size, char* buf, int* fat, char* img);
struct FILEINFO* file_search(char* name, struct FILEINFO* finfo, int max);
