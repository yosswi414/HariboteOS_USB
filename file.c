#include "file.h"
#include "general.h"
#include "mylibgcc.h"

void file_readfat(int* fat, unsigned char* img) {
    for (int i = 0, j = 0; i < 2880; i += 2, j += 3) {
        fat[i + 0] = (img[j + 0] >> 0 | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
    }
    return;
}

void file_loadfile(int clustno, int size, char* buf, int* fat, char* img) {
    while (TRUE) {
        if (size <= 512) {
            for (int i = 0; i < size; ++i) buf[i] = img[clustno * 512 + i];
            break;
        }
        for (int i = 0; i < 512; ++i) {
            buf[i] = img[clustno * 512 + i];
        }
        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
    return;
}

struct FILEINFO* file_search(char* name, struct FILEINFO* finfo, int max) {
    char s[16];
    for (int i = 0; i < 11; ++i) s[i] = ' ';

    for (int i = 0, j = 0; name[i]; ++i) {
        if (j >= 11) return NULL;
        if (name[i] == '.')
            j = max(j, 8);
        else
            s[j++] = toupper(name[i]);
    }

    for (int i = 0; i < max; ++i){
        if (!finfo[i].name[0]) break;
        if(!(finfo[i].type & 0x18)){
            if (!strncmp(finfo[i].name, s, 11)) return finfo + i;
        }
    }

    return NULL;
}