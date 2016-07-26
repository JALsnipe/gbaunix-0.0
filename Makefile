# Amit Singh
# http://www.kernelthread.com
#
# 5th Edition UNIX on the Nintendo Gameboy Advance
#

# Host: Mac OS X

CROSSTOOLS_PREFIX = arm-elf
CROSSTOOLS_PATH   = /usr/local/devkitARM
AS                = $(CROSSTOOLS_PREFIX)-as
CC                = $(CROSSTOOLS_PREFIX)-gcc
LD                = $(CROSSTOOLS_PREFIX)-ld
OBJCOPY           = $(CROSSTOOLS_PREFIX)-objcopy
INCLUDES          = -I$(CROSSTOOLS_PATH)/$(CROSSTOOLS_PREFIX)/include
LOCAL_INCLUDES    =
ASFLAGS           = $(INCLUDES)

# -DGAMEBOY       mandatory
# -DGAMEBOY_DMA
# -DGAMEBOY_FASTDATA
# -DGAMEBOY_KEYPAD_IRQ
# -DGAMEBOY_PDP_CIS
# -DGAMEBOY_WCACHE
#
# GAMEBOY_FLAGS     = -DGAMEBOY -DGAMEBOY_DMA -DGAMEBOY_KEYPAD_IRQ
GAMEBOY_FLAGS     = -DGAMEBOY

# COPTFLAG        = -O3 -fnew-ra -funit-at-a-time
# COPTFLAG        = -Os
COPTFLAG          = -O3
CFLAGS            = $(GAMEBOY_FLAGS) $(INCLUDES) $(LOCAL_INCLUDES) $(COPTFLAG) \
                    -mthumb-interwork -specs=gba.specs -mcpu=arm7tdmi \
                    -mtune=arm7tdmi
CFLAGS_ARM        = $(GAMEBOY_FLAGS) $(INCLUDES) $(LOCAL_INCLUDES) $(COPTFLAG) \
                    -marm -mthumb-interwork -specs=gba.specs -mcpu=arm7tdmi \
                    -mtune=arm7tdmi
LDFLAGS           = -Map /tmp/gba.map -lm
#LDFLAGS           = -lm
OBJCOPYFLAGS      = -v -O binary

GBAUNIXV5_ELF     = unixv5
GBAUNIXV5_TMP     = unixv5.tmp
GBAUNIXV5_ROM     = unixv5.gba
UNIXV5_DISK       = disks/unixv5.dsk

OBJS              = pdp11_cpu.o pdp11_fp.o pdp11_lp.o pdp11_rk.o pdp11_rl.o \
                    pdp11_rp.o pdp11_rx.o pdp11_stddev.o pdp11_sys.o \
                    pdp11_tm.o scp.o gba/gba_fsio.o gba/gba_fsio_core.o \
                    gba/gba_intr.o gba/gba_single_intr.o gba/gba_sys.o \
                    gba/gba_tty.o gba/gba_tty_charmap.o

default: all

gba/gba_fsio_core.o: gba/gba_fsio_core.c gba/gba_fsio.h gba/gba_dma.h \
                     gba/gba_unix.h
	$(CC) $(CFLAGS_ARM) -c -o $@ $<

gba/gba_fsio.o: gba/gba_fsio.c gba/gba_fsio.h gba/gba_dma.h gba/gba_sys.h \
                     gba/gba_unix.h

gba/gba_intr.o: gba/gba_intr.c gba/gba_intr.h gba/gba_gfx.h

gba/gba_single_intr.o: gba/gba_single_intr.s

gba/gba_sys.o: gba/gba_sys.c gba/gba_intr.h gba/gba_sys.h

gba/gba_tty.o: gba/gba_tty.c sim_defs.h gba/gba_io.h gba/gba_kbd.h \
               gba/gba_dma.h gba/gba_tty.h

gba/gba_tty_charmap.o: gba/gba_tty_charmap.c gba/gba_tty.h

# --

gba/gba_dma.h: gba/gba_io.h gba/gba_intr.h

gba/gba_gfx.h: gba/gba_io.h

gba/gba_intr.h: gba/gba_io.h

gba/gba_tty.h: gba/gba_io.h

# --

pdp11_cpu.o: pdp11_cpu.c pdp11_defs.h gba/gba_io.h

pdp11_fp.o: pdp11_fp.c pdp11_defs.h gba/gba_io.h

pdp11_lp.o: pdp11_lp.c pdp11_defs.h

pdp11_rk.o: pdp11_rk.c pdp11_defs.h

pdp11_rl.o: pdp11_rl.c pdp11_defs.h

pdp11_rp.o: pdp11_rp.c pdp11_defs.h

pdp11_rx.o: pdp11_rx.c pdp11_defs.h

pdp11_stddev.o: pdp11_stddev.c pdp11_defs.h

pdp11_sys.o: pdp11_sys.c pdp11_defs.h

pdp11_tm.o: pdp11_tm.c pdp11_defs.h

scp.o: scp.c gba/gba_fsio.h gba/gba_intr.h sim_defs.h

# --

pdp11_defs.h: sim_defs.h

sim_defs.h: gba/gba_fsio.h

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.s.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(GBAUNIXV5_ROM): $(GBAUNIXV5_TMP) $(UNIXV5_DISK)
	@rm gba/gba_fsio_core.o
	@echo "#define GBA_UNIXOFFSET `cat $(GBAUNIXV5_TMP) | wc -c`" > gba/gba_unix.h
	@echo "#define GBA_UNIXSIZE `cat $(UNIXV5_DISK) | wc -c`" >> gba/gba_unix.h
	$(CC) $(CFLAGS) -c gba/gba_fsio_core.c -o gba/gba_fsio_core.o
	$(CC) $(CFLAGS) -o $(GBAUNIXV5_ELF) $(OBJS) -Wl,$(LDFLAGS)
	$(OBJCOPY) $(OBJCOPYFLAGS) $(GBAUNIXV5_ELF) $(GBAUNIXV5_TMP)
	cat $(GBAUNIXV5_TMP) $(UNIXV5_DISK) > $@
	@du -h $@

$(GBAUNIXV5_TMP): $(GBAUNIXV5_ELF)
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

$(GBAUNIXV5_ELF): $(OBJS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS) -Wl,$(LDFLAGS)

.PHONY: all clean realclean

all: $(GBAUNIXV5_ROM)

clean:
	rm -f $(OBJS) *.d *.i *.s $(GBAUNIXV5_ELF) $(GBAUNIXV5_TMP)

realclean: clean
	rm -f $(GBAUNIXV5_ROM)
