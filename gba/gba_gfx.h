#ifndef _GBA_GFX_H_
#define _GBA_GFX_H_

#include "gba_io.h"

#define	REG_DISPCNT    *(vu16 *)(GBA_IOBASE + 0x00)
#define	REG_DISPSTAT   *(vu16 *)(GBA_IOBASE + 0x04)
#define LCDC_VBL_FLAG  (1 << 0)
#define LCDC_HBL_FLAG  (1 << 1)
#define LCDC_VCNT_FLAG (1 << 2)
#define LCDC_VBL       (1 << 3)
#define LCDC_HBL       (1 << 4)
#define LCDC_VCNT      (1 << 5)
#define VCOUNT(m)      ((m) << 8)

#endif /* _GBA_GFX_H_ */
