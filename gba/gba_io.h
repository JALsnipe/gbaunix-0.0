#ifndef _GBA_BASE_H_
#define _GBA_BASE_H_

/* Derived from libgba */

typedef	unsigned char       u8;
typedef	unsigned short int u16;
typedef	unsigned int       u32;
typedef	signed char         s8;
typedef	signed short int   s16;
typedef	signed int         s32;
typedef	volatile u8        vu8;
typedef	volatile u16      vu16;
typedef	volatile u32      vu32;
typedef	volatile s8        vs8;
typedef	volatile s16      vs16;
typedef	volatile s32      vs32;

#define N2BIT(N) (1 << N)

#define	GBA_EWRAM   0x02000000
#define	GBA_IWRAM   0x03000000
#define	GBA_IOBASE  0x04000000
#define GBA_PALETTE 0x05000000
#define	GBA_VRAM    0x06000000
#define	GBA_SRAM    0x0E000000

#if defined (__thumb__)
#define	gba_syscall(N) asm ("SWI	"#N"\n" :::  "r0", "r1", "r2", "r3")
#else
#define	gba_syscall(N) asm ("SWI	"#N" << 16\n" :::"r0", "r1", "r2", "r3")
#endif

#ifdef GAMEBOY_FASTDATA
#define GBA_IWRAM_DATA __attribute__ ((section(".iwram")))
#else
#define GBA_IWRAM_DATA
#endif

#endif /* _GBA_BASE_H_ */
