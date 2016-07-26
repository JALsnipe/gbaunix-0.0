#ifndef _GBA_SYS_H_
#define _GBA_SYS_H_

void gba_reset(void);
void gba_stop(void);
void gba_abort(char *msg);
inline void gba_intr_wait(u32 rflag, u32 iflag);
inline void gba_cpufastset(u32 src, u32 dst, u32 lenmode);

#endif /* _GBA_SYS_H_ */

