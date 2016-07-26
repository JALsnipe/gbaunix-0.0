#include <stdio.h>

#define GBA_FSIO_CORE

__attribute__ ((long_call)) void *malloc(size_t);
__attribute__ ((long_call)) void *realloc(void *, size_t);
__attribute__ ((long_call)) void *memmove(void *, void *, size_t);
#ifndef GAMEBOY_DMA
__attribute__ ((long_call)) void *memcpy(void *, const void *, size_t);
#endif

#include "gba_fsio.h"
#include "gba_dma.h"
#include "gba_unix.h"

const char *unixv5 = (char *)(0x08000000 + GBA_UNIXOFFSET);

int
gba_fseek(FILE *stream, long offset, int whence)
{
    if (filestable[(int)stream] == 1) {
        fpos[(int)stream] = offset;
        return 0;
    } else {
        return -1;
    }
}

void
gba_rewind(FILE *stream)
{
    if (filestable[(int)stream] == 1)
        fpos[(int)stream] = 0;
}

long
gba_ftell(FILE *stream)
{
    if (filestable[(int)stream] == 1)
        return fpos[(int)stream];
    else
        return -1;
}

/*
 *          r---            r---                 r--- 
 *   w\\\\\       a        w\\\\\    b        w\\\\\     c
 *
 *   r---                  r-----             r-----
 *        w\\\\\  d         w\\\     e            w\\\   f
 */
size_t
gba_fread(void *_ptr, size_t size, size_t nmemb, FILE *stream)
{
    int i, diff, overlapped = -1;
    int base;
    int nbytes;
    int basep = fpos[(int)stream];
    int nbytesp =  size * nmemb;
    unsigned char *ptr = (unsigned char *)_ptr;

    for (i = 0; i < di; i++) {
        base = D[i].base;
        nbytes = D[i].nbytes;
        if (basep >= base) {
            if (basep > (base + nbytes)) /* a */
                continue;
            if ((basep + nbytesp) <= (base + nbytes)) { /* b */
                if (D[i].data == NULL) {
                    gba_abort("Disk error. Halted.\n");
                    return 0;
                }
                diff = basep - base;
                gba_memcpy((char *)ptr, (char *)(D[i].data + diff), nbytesp);
                fpos[(int)stream] += nbytesp;
                return nmemb;
            } else { /* c */
                overlapped = i;
                break;
            }
        } else { /* (basep < base) */
            if ((basep + nbytesp) < base) /* d */
                continue;
            overlapped = i;
            break; 
        } 
    }

    if (overlapped == -1) {
        if ((fpos[(int)stream] + nbytesp) >= GBA_UNIXSIZE) {
            nbytesp = GBA_UNIXSIZE - nbytesp - 1;
            gba_memcpy(ptr, unixv5 + fpos[(int)stream], nbytesp);
            fpos[(int)stream] += nbytesp;
            return EOF;
        }

        gba_memcpy(ptr, unixv5 + fpos[(int)stream], nbytesp);
        fpos[(int)stream] += nbytesp;

        return nmemb;
    }

    if (basep >= base) { /* c */
        diff = (basep + nbytesp) - (base + nbytes);
        gba_memcpy(ptr, D[i].data + (base + nbytes - basep), nbytesp - diff);
        gba_memcpy(ptr + (nbytesp - diff), unixv5 + basep + (nbytesp - diff), diff);
    } else if ((basep + nbytesp) > (base + nbytes)) { /* e */
        diff = base - basep;
        gba_memcpy(ptr, unixv5 + basep, base - basep); 
        gba_memcpy(ptr + base - basep, D[i].data, nbytes); 
        diff = (basep + nbytesp) - (base + nbytes);
        gba_memcpy(ptr + base - basep + nbytes, unixv5 + base - basep + nbytes, diff);
    } else { /* f */
        diff = (basep + nbytesp) - base; 
        gba_memcpy(ptr, unixv5 + basep, base - basep);
        gba_memcpy(ptr + base - basep, D[i].data, diff);
    }

    fpos[(int)stream] += nbytesp;
    return nmemb;
}

/*
 *          c---            c---                 c---
 *   w\\\\\       a        w\\\\\    b        w\\\\\     c
 *  
 *   c---                  c-----             c-----
 *        w\\\\\  d         w\\\     e            w\\\   f
 */

size_t
gba_fwrite(void *_ptr, size_t size, size_t nmemb, FILE *stream)
{
    int i, overlapped = -1;
    int base, nbytes;
    int basep, nbytesp;
    unsigned char *tmp;
    unsigned char *ptr = (unsigned char *)_ptr;

    basep = fpos[(int)stream];
    nbytesp =  size * nmemb;

    for (i = 0; i < di; i++) {

        base = D[i].base;
        nbytes = D[i].nbytes;

        if ((basep > (base + nbytes)) || ((basep + nbytesp) < base)) /* a, d */
            continue;

        if ((basep >= base) && ((basep + nbytesp) <= (base + nbytes))) { /* b */
            /* reuse */
            gba_memcpy(D[i].data + basep - base, ptr, nbytesp);
            fpos[(int)stream] += nbytesp;
            return nmemb;
        }

        overlapped = i; 
        break;
    }

    if (overlapped == -1) { /* a, d */
        D[di].base = basep;
        D[di].nbytes = nbytesp;
        D[di].data = (unsigned char *)malloc(nbytesp);
        if (D[di].data == NULL) {
            gba_abort("Memory exhausted. Halting.\n"); 
            return 0;
        }
        gba_memcpy(D[di].data, ptr, nbytesp);
        di++;
        fpos[(int)stream] += nbytesp;
        return nmemb; 
    }

    if (basep >= base) { /* c */
        tmp = (unsigned char *)realloc(D[i].data, basep - base + nbytesp);
        if (tmp == NULL) {
            gba_abort("Memory exhausted. Halting.\n"); 
            return 0;
        }
        D[i].data = tmp;
        D[i].nbytes = basep - base + nbytesp;
        gba_memcpy(D[i].data + basep - base, ptr, nbytesp);
    } else if ((basep + nbytesp) > (base + nbytes)) { /* e */
        tmp = (unsigned char *)realloc(D[i].data, nbytesp);
        if (tmp == NULL) {
            gba_abort("Memory exhausted. Halting.\n"); 
            return 0;
        }
        D[i].data = tmp;
        D[i].nbytes = nbytesp;
        D[i].base = basep; 
        gba_memcpy(D[i].data, ptr, nbytesp);
    } else { /* f */
        tmp = (unsigned char *)realloc(D[i].data, base - basep + nbytes);
        if (tmp == NULL) {
            gba_abort("Memory exhausted. Halting.\n"); 
            return 0;
        }
        D[i].data = tmp;
        D[i].nbytes = base - basep + nbytes; 
        D[i].base = basep;
        memmove(D[i].data + base - basep, D[i].data, nbytes); /* XXX */
        gba_memcpy(D[i].data, ptr, nbytesp);
    }

    fpos[(int)stream] += nbytesp;

    return nmemb;
}

int
gba_getc(FILE *stream)
{
    if (fpos[(int)stream] < GBA_UNIXSIZE) {
        fpos[(int)stream] = fpos[(int)stream] + 1;
        return unixv5[fpos[(int)stream] - 1];
    } else
        return EOF;
}
