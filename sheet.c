#include "sheet.h"
#include "general.h"
#include "memory.h"

struct SHTCTL* shtctl_init(struct MEMMAN* memman, unsigned char* vram, int xsize, int ysize) {
    struct SHTCTL* ctl;
    ctl = (struct SHTCTL*)memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (!ctl) return ctl;

    ctl->map = (unsigned char*)memman_alloc_4k(memman, xsize * ysize);
    if (!ctl->map) {
        memman_free_4k(memman, (int)ctl, sizeof(struct SHTCTL));
        return ctl;
    }

    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for (int i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = SHEET_VACANT;
        ctl->sheets0[i].ctl = ctl;
    }
    return ctl;
}

struct SHEET* sheet_alloc(struct SHTCTL* ctl) {
    struct SHEET* sht;
    for (int i = 0; i < MAX_SHEETS; i++) {
        if (ctl->sheets0[i].flags) continue;
        sht = &ctl->sheets0[i];
        sht->flags = SHEET_USE;
        sht->height = -1;
        return sht;
    }
    return 0;
}

void sheet_setbuf(struct SHEET* sht, unsigned char* buf, int xsize, int ysize, int col_inv) {
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_refreshsub(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    int vx, vy;
    unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
    struct SHEET* sht;
    vx0 = max(vx0, 0);
    vy0 = max(vy0, 0);
    vx1 = min(vx1, ctl->xsize);
    vy1 = min(vy1, ctl->ysize);
    for (int h = h0; h <= h1; h++) {
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;
        int bx0 = max(vx0 - sht->vx0, 0);
        int by0 = max(vy0 - sht->vy0, 0);
        int bx1 = min(vx1 - sht->vx0, sht->bxsize);
        int by1 = min(vy1 - sht->vy0, sht->bysize);
        for (int by = by0; by < by1; by++) {
            vy = sht->vy0 + by;
            for (int bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx;
                if (map[vy * ctl->xsize + vx] == sid)
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
            }
        }
    }
    return;
}

void sheet_updown(struct SHEET* sht, int height) {
    struct SHTCTL* ctl = sht->ctl;
    int old = sht->height;
    height = clamp(height, -1, ctl->top + 1);
    sht->height = height;

    if (old > height) {
        if (height >= 0) {
            for (int h = old; h > height; h--) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
        } else {
            if (ctl->top > old) {
                for (int h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;
            sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
        }
    } else if (old < height) {
        if (old >= 0) {
            for (int h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {
            for (int h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }
    return;
}

void sheet_refresh(struct SHEET* sht, int bx0, int by0, int bx1, int by1) {
    if (sht->height >= 0) {
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    }
    return;
}

void sheet_slide(struct SHEET* sht, int vx0, int vy0) {
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        sheet_refreshmap(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

void sheet_free(struct SHEET* sht) {
    if (sht->height >= 0) sheet_updown(sht, -1);
    sht->flags = 0;
    return;
}

void sheet_refreshmap(struct SHTCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int bx0, by0, bx1, by1, vx, vy;
    unsigned char *buf, sid, *map = ctl->map;
    struct SHEET* sht;
    vx0 = max(vx0, 0);
    vy0 = max(vy0, 0);
    vx1 = min(vx1, ctl->xsize);
    vy1 = min(vy1, ctl->ysize);
    for (int h = h0; h <= ctl->top; ++h) {
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0;
        buf = sht->buf;
        bx0 = max(vx0 - sht->vx0, 0);
        by0 = max(vy0 - sht->vy0, 0);
        bx1 = min(vx1 - sht->vx0, sht->bxsize);
        by1 = min(vy1 - sht->vy0, sht->bysize);
        for (int by = by0; by < by1; ++by) {
            vy = sht->vy0 + by;
            for (int bx = bx0; bx < bx1; ++bx) {
                if (buf[by * sht->bxsize + bx] == sht->col_inv) continue;
                vx = sht->vx0 + bx;
                map[vy * ctl->xsize + vx] = sid;
            }
        }
    }
    return;
}