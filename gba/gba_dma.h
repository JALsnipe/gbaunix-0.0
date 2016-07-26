#ifndef _GBA_DMA_H_
#define _GBA_DMA_H_

#include "gba_io.h"
#include "gba_intr.h"

#define GBA_DMA3_SRCREG	*((vu32 *)0x40000D4)
#define GBA_DMA3_DSTREG *((vu32 *)0x40000D8)
#define GBA_DMA3_CNTREG *((vu16 *)0x40000DC)
#define GBA_DMA3_CTLREG *((vu16 *)0x40000DE)

#define gba_memcpy_c(dst, src, cnt) gba_cpufastset(src, dst, (cnt >> 2))
#define gba_memset_c(dst, c, cnt)   gba_cpufastset(c, dst, ((1 << 24) | (cnt >> 2)))

#ifdef GAMEBOY_DMA

#define gba_memcpy32(dst, src, cnt) \
    { \
        GBA_DMA3_CNTREG = (u16)((cnt) >> 2); \
        GBA_DMA3_CTLREG = (u16)(1 << 10); \
        GBA_DMA3_DSTREG = (u32)(dst); \
        GBA_DMA3_SRCREG = (u32)(src); \
        GBA_DMA3_CTLREG |= (u16)(1 << 15); \
    }

#define gba_memset32(dst, src, cnt) \
    { \
        GBA_DMA3_CNTREG = (u16)((cnt) >> 2); \
        GBA_DMA3_CTLREG = (u16)((1 << 10) | (1 << 8)); \
        GBA_DMA3_DSTREG = (u32)(dst); \
        GBA_DMA3_SRCREG = (u32)(src); \
        GBA_DMA3_CTLREG |= (u16)(1 << 15); \
    }

#define gba_memcpy(dst, src, cnt) \
    { \
        if (cnt) { /* UNIX _is_ doing zero byte reads! */ \
            u16 d_1 = (u32)(dst) & 0x1; \
            u16 s_1 = (u32)(src) & 0x1; \
            u16 c_1 = (u32)(cnt) & 0x1; \
            if (d_1 || s_1 || c_1) { /* odd */ \
                memcpy((dst), (src), (cnt)); \
            } else { \
                u16 d_3 = (u32)(dst) & 0x3; \
                u16 s_3 = (u32)(src) & 0x3; \
                u16 c_3 = (u32)(cnt) & 0x3; \
                if (!d_3 && !s_3 && !c_3) { /* x 4 */ \
                    GBA_DMA3_CNTREG = ((u16)((cnt) >> 2)); \
                    GBA_DMA3_CTLREG = (u16)(1 << 10); \
                } else { /* x 2 */ \
                    GBA_DMA3_CNTREG = ((u16)((cnt) >> 1)); \
                    GBA_DMA3_CTLREG = 0; \
                } \
                GBA_DMA3_DSTREG = ((u32)(dst)); \
                GBA_DMA3_SRCREG = ((u32)(src)); \
                GBA_DMA3_CTLREG |= (u16)(1 << 15); \
            } \
        } \
    }
#else
#define gba_memcpy(dst, src, cnt) memcpy(dst, src, cnt)
#endif

#endif /* _GBA_DMA_H_ */
