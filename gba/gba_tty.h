#ifndef _GBA_TTY_H_
#define _GBA_TTY_H_

#include "gba_io.h"

struct charmap {
    unsigned char bitmap[8];
};

/* Coordinates */
#define GBA_XMAX       32
#define GBA_XLEN       30
#define GBA_YMAX       32
#define GBA_YLEN       20

#define GBA_KEYBASE    *(volatile unsigned short *)(0x4000130)
#define GBA_KEY_A      0x1
#define GBA_KEY_B      0x2
#define GBA_KEY_SELECT 0x4
#define GBA_KEY_START  0x8
#define GBA_KEY_RIGHT  0x10
#define GBA_KEY_LEFT   0x20
#define GBA_KEY_UP     0x40
#define GBA_KEY_DOWN   0x80
#define GBA_KEY_R      0x100
#define GBA_KEY_L      0x200
#define GBA_KEY_NONE   0x0
#define GBA_KEY_ALL    0x3ff

#define GBA_ISKEY_UP(key)   !(~(GBA_KEYBASE | 0xfc00) & (key))
#define GBA_ISKEY_DOWN(key) ~(GBA_KEYBASE | 0xfc00) & (key)
#define GBA_KEY_GET         (unsigned long)(~(GBA_KEYBASE | 0xfc00))

extern const struct charmap gbatxt_charmap[];

void gba_tty_init(void);
void gba_tty_print(const char *s);
void gba_tty_inittext(void);
void gba_tty_setxy(int x, int y);
void gba_tty_scroll(void);
void gba_tty_putc(int c);

#endif
