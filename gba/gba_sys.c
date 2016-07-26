#include "gba_intr.h"
#include "gba_sys.h"
#include "gba_tty.h"

void
gba_reset(void)
{
    gba_syscall(0x0);
}

void
gba_stop(void)
{
    gba_syscall(0x3);
}

void
gba_abort(char *msg)
{
    gba_tty_print(msg);
    gba_stop();
}

inline void
gba_intr_wait(u32 rflag, u32 iflag)
{
    gba_syscall(0x4);
}

inline void
gba_cpufastset(u32 src, u32 dst, u32 lenmode)
{
    gba_syscall(0xc);
}
