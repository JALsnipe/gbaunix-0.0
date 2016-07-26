#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gba_dma.h"
#include "gba_fsio.h"
#include "gba_sys.h"
#include "gba_unix.h"

long fpos[GBA_MAXFILES];
int filestable[GBA_MAXFILES];
dcache_t D[DI_MAX];
int di = 0;

void
gba_fsio_init(void)
{
    int i;
#ifdef GAMEBOY_WCACHE
    unsigned char *Wbuf;
#endif

    for (i = 0; i < DI_MAX; i++) {
        D[i].base = 0;
        D[i].nbytes = 0;
        D[i].data = (unsigned char *)0;
    }

    for (i = 0; i < GBA_MAXFILES; i++) {
        fpos[i] = 0;
        filestable[i] = 0;
    }

#ifdef GAMEBOY_WCACHE
    D[0].base = 0;       /* XXX */
    D[0].nbytes = 24576; /* XXX */
    D[1].base = 2048000; /* XXX */
    D[1].nbytes = 16384; /* XXX */

    Wbuf = (unsigned char *)malloc(D[0].nbytes + D[1].nbytes);
    if (Wbuf == NULL) {
        gba_abort("Disk error. Halted.\n");
        return;
    }

    D[0].data = Wbuf;
    D[1].data = Wbuf + D[0].nbytes;

    gba_memcpy(D[0].data, unixv5 + D[0].base, D[0].nbytes);
    gba_memcpy(D[1].data, unixv5 + D[1].base, D[1].nbytes);

    di = 2;
#endif

    filestable[0] = 1;
    filestable[1] = 1;
    filestable[2] = 1;
}

FILE *
gba_fopen(const char *path, const char *mode)
{
    int i;

    for (i = 0; i < GBA_MAXFILES; i++)
        if (filestable[i] == 0)
            break;

    if (i == GBA_MAXFILES)
        return (void *)0;

    fpos[i] = 0;
    filestable[i] = 1;

    return (void *)i;
}

int
gba_fclose(FILE *stream)
{
    filestable[(int)stream] = 0;
    fpos[(int)stream] = 0;
    return 0;
}

char *
gba_fgets(char *cptr, int size, FILE *s)
{
    static int cmdidx = 0;

    switch (cmdidx) {
    case 0:
        cmdidx++;
        sprintf(cptr, "set cpu 18b\n");
        return cptr;
    break;

    case 1:
        cmdidx++;
#ifdef GAMEBOY_PDP_CIS
        sprintf(cptr, "set cpu CIS\n");
#else
        sprintf(cptr, "set cpu 18b\n");
#endif
        return cptr;
    break;

    case 2:
        cmdidx++;
        sprintf(cptr, "att rk0 dsk\n");
        return cptr;
    break;

    case 3:
        cmdidx++;
        sprintf(cptr, "boot rk\n");
        return cptr;
    break;

    }

    gba_stop();

    return (char *)0;
}

void *
gba_calloc(size_t nmemb, size_t size)
{
  void *ptr;

  size *= nmemb;
  ptr = (void *)malloc(size);
  if (ptr)
    memset(ptr, 0, size);

  return ptr;
}

int
gba_fputs(const char *str, FILE *stream)
{
    gba_tty_print(str);
    return 0;
}
