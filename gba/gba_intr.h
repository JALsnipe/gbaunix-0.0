#ifndef _GBA_INTR_H_
#define _GBA_INTR_H_

#include "gba_io.h"

typedef void (* intr_func_t)(void);

typedef struct intr_table_s {
    intr_func_t handler;
    u32         mask;
} intr_table_t;

typedef enum {
    INT_VBLANK,
    INT_HBLANK,
    INT_VCOUNT,
    INT_TIMER0,
    INT_TIMER1,
    INT_TIMER2,
    INT_TIMER3,
    INT_SERIAL,
    INT_DMA0,
    INT_DMA1,
    INT_DMA2,
    INT_DMA3,
    INT_KEYPAD,
    INT_GAMEPAK,
    INT_COUNT,
    INT_ALL = 0x7fff,
} intr_type_t;

#define INT_VECTOR  *(intr_func_t *)(0x03007ffc)
#define	REG_IME     *(vu16 *)(GBA_IOBASE + 0x208)
#define	REG_IE      *(vu16 *)(GBA_IOBASE + 0x200)
#define	REG_IF      *(vu16 *)(GBA_IOBASE + 0x202)
#define REG_BIOSIF  *(vu16 *)0x03fffff8
#define REG_P1CNT   *(vu16 *)0x04000132
#define REG_WAITCNT *(vu16 *)0x04000204
#define REG_UND_IMC *(vu32 *)0x04000800

#define IE_VBL      (1 << 0)
#define IE_HBL      (1 << 1)
#define IE_VCNT     (1 << 2)
#define IE_TIMER0   (1 << 3)
#define IE_TIMER1   (1 << 4)
#define IE_TIMER2   (1 << 5)
#define IE_TIMER3   (1 << 6)
#define IE_SERIAL   (1 << 7)
#define IE_DMA0     (1 << 8)
#define IE_DMA1     (1 << 9)
#define IE_DMA2     (1 << 10)
#define IE_DMA3     (1 << 11)
#define IE_KEYPAD   (1 << 12)
#define IE_GAMEPAK  (1 << 13)

extern intr_table_t intr_table[];

void gba_disable_interrupt(intr_type_t intr);
void gba_enable_interrupt(intr_type_t intr);
void gba_intr_init(void);
void _gba_intr_main(void);
void gba_intr_wait(u32 rflag, u32 iflag);
void gba_set_interrupt(intr_type_t intr, intr_func_t func);

#define	GBA_VBLANK_intr_wait() gba_syscall(0x5)

#endif /* _GBA_INTR_H_ */
