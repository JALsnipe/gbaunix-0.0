#include "../sim_defs.h"
#include "gba_io.h"
#include "gba_kbd.h"
#include "gba_dma.h"
#include "gba_tty.h"

int gba_tty_X;
int gba_tty_Y;
char gsbuf[1024];

void
gba_tty_inittext(void)
{
    unsigned short *vp;
    struct charmap *cp;
    unsigned char bits;
    int i, j, r, b;

    vp = (unsigned short *)GBA_VRAM;

    for (j = 0; (j < 2); j++) {
        cp = (struct charmap *) &gbatxt_charmap[0];
        for (i = 0; (i < 128); i++, cp++) {
            for (r = 0; (r < 8); r++) {
                bits = cp->bitmap[r];
                    for (b = 0; (b < 8); b += 2) {
                        *vp++ = ((bits & (0x80>>b)) ? 1 : 0) |
                          ((bits & (0x40>>b)) ? 0x100 : 0);
                    }
            }
        }
    }
}

void
gba_tty_init(void)
{
    int i;
    volatile unsigned short *pp;

    /* Enable mode 0 */
    *((unsigned short *) (GBA_IOBASE + 0x00)) = 0x0100; /*MODE0|BG0*/
    *((unsigned short *) (GBA_IOBASE + 0x08)) = 0x1080;

    /* Default palete, everything is white :-) */
    pp = (volatile unsigned short *)GBA_PALETTE;
    for (i = 255; i; i--)
        pp[i] = 0x7fff;
    pp[0] = 0;

    gba_tty_X = 0;
    gba_tty_Y = 0;

    gba_tty_inittext();
}

void
gba_tty_setxy(int x, int y)
{
    if ((x >= 0) && (x < GBA_XLEN) && (y >= 0) && (y < GBA_YLEN)) {
        gba_tty_X = x;
        gba_tty_Y = y;
    }
}

void
gba_tty_scroll(void)
{
    unsigned short *sp, *dp;
    int len;

    dp = (unsigned short *)(GBA_VRAM + 0x8000);
    sp = dp + GBA_XMAX;
    len = (GBA_XMAX * (GBA_YLEN - 1));

    while (len--)
        *dp++ = *sp++;

    for (len = GBA_XMAX; len; len--)
        *dp++ = 0;
}

void
gba_tty_putc(int c)
{
    unsigned short *vp;

    vp = (unsigned short *)(GBA_VRAM + 0x8000);
    vp += (gba_tty_Y * GBA_XMAX) + gba_tty_X;
    *vp = c;

    gba_tty_X++;
    if ((gba_tty_X >= GBA_XLEN) || (c == '\n')) {
        gba_tty_X = 0;
        gba_tty_Y++;
        if (gba_tty_Y >= GBA_YLEN) {
            gba_tty_scroll();
            gba_tty_Y = GBA_YLEN - 1;
        }
    }

    switch (c) { 
#if 0
        case 9:
            gba_tty_X += 4;
        break;
#endif

        case 11:
            gba_tty_Y += 2;
        break;

        case '\r':
            gba_tty_X = 0;
            gba_tty_Y++;
        break; 
    }
}

void
gba_tty_print(const char *s)
{
    for (; *s; s++)
        gba_tty_putc(*s);	
}

int
gba_puts(char *s)
{
    gba_tty_print(s);
    return 0;
}

int
gba_putsnl(char *s)
{
    gba_tty_print(s);
    gba_tty_print("\n");
    return 0;
}

#ifdef GAMEBOY_KEYPAD_IRQ
extern int readyforinput;
#endif

int
gba_kbd_read(int d, void *_buf, int nbytes)
{
    static int inputidx = 0;
    static int chridx = 0;
#ifndef GAMEBOY_KEYPAD_IRQ
    static int readyforinput = 0;
#endif
    unsigned char *buf = (unsigned char *)_buf;

#ifdef GAMEBOY_KEYPAD_IRQ
    if (!readyforinput)
        return -1;

    if (!gba_kbdinput[inputidx]) { /* commands exhausted */
        printf("\nRebooting system ...\n");
        gba_reset();
    }
#else
    if (gba_kbdinput[inputidx] == NULL)
        return -1;

    if (!readyforinput) {
        if (GBA_ISKEY_DOWN(GBA_KEY_START))
            readyforinput = 1;
        else
            return -1;
    }
#endif

    if (!*(gba_kbdinput[inputidx] + chridx)) {
        inputidx++;
        chridx = 0;
        readyforinput = 0;
        return -1;
    }

    buf[0] = *(gba_kbdinput[inputidx] + chridx);
    chridx++;

    return 1;
}

/* tty */

int32 sim_int_char = 005;

t_stat ttinit (void)
{
    return SCPE_OK;
}

t_stat ttrunstate (void)
{
    return SCPE_OK;
}

t_stat ttcmdstate (void)
{
    return SCPE_OK;
}

t_stat ttclose (void)
{
    return ttcmdstate();
}

t_stat sim_poll_kbd (void)
{
    int status;
    unsigned char buf[1];
    status = gba_kbd_read (0, buf, 1);
    if (status != 1)
        return SCPE_OK;
    else
        return (buf[0] | SCPE_KFLAG);
}

t_stat sim_putchar (int32 out)
{
    char c = out;

    gba_tty_putc(c);

    return SCPE_OK;
}
