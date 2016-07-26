#include "gba_intr.h"
#include "gba_gfx.h"

#ifdef GAMEBOY_KEYPAD_IRQ
volatile int readyforinput = 0;
#endif

intr_table_t intr_table[INT_COUNT];

void
gba_null_isr(void)
{
    /* nothing */
}

#ifdef GAMEBOY_KEYPAD_IRQ
void
gba_keypad_isr(void)
{
    int interrupts = REG_IF;
    REG_IME = 0;
    readyforinput = 1;
    REG_BIOSIF |= interrupts;
    REG_IF = interrupts;
    REG_IME = 1;
}
#endif

#ifdef GAMEBOY_DMA3_IRQ
void
gba_dma3_isr(void)
{
    int interrupts = REG_IF;
    REG_IME = 0;
    REG_BIOSIF |= interrupts;
    REG_IF = interrupts;
    REG_IME = 1;
}
#endif

void
gba_intr_init()
{
    int i = 0;

    REG_WAITCNT |= (1 << 14);
#if 0
    REG_UND_IMC |= ((1 << 27) | (1 << 26) | (1 << 25) | (0 << 24));
#endif

    for (; i < INT_COUNT; i ++) {
        intr_table[i].handler = gba_null_isr;
        intr_table[i].mask = 0;
    }

    INT_VECTOR = _gba_intr_main;

#ifdef GAMEBOY_KEYPAD_IRQ
    REG_P1CNT = (1 << 14) | (1 << 3);
    gba_set_interrupt(INT_KEYPAD, gba_keypad_isr);
    gba_enable_interrupt(INT_KEYPAD);
#endif

#ifdef GAMEBOY_DMA3_IRQ
    gba_set_interrupt(INT_DMA3, gba_dma3_isr);
    gba_enable_interrupt(INT_DMA3);
#endif

}

void
gba_set_interrupt(intr_type_t intr, intr_func_t func)
{
    int i, found = 0;
    u32 mask = N2BIT(intr);

    for (i = 0; i < INT_COUNT; i++)
        if (!intr_table[i].mask || intr_table[i].mask == mask) {
            found = 1;
            break;
        }

    if (found) {
        intr_table[i].handler = func;
        intr_table[i].mask = mask;
    }
}

void
gba_enable_interrupt(intr_type_t intr)
{
    REG_IME = 0;

    switch (intr) {

        case INT_VBLANK:
            REG_DISPSTAT |= LCDC_VBL;
            REG_IE |= N2BIT(intr);
        break;

        case INT_HBLANK:
            REG_DISPSTAT |= LCDC_HBL;
            REG_IE |= N2BIT(intr);
        break;

        case INT_VCOUNT:
            REG_DISPSTAT |= LCDC_VCNT;
            REG_IE |= N2BIT(intr);
        break;

        case INT_TIMER0:
        case INT_TIMER1:
        case INT_TIMER2:
        case INT_TIMER3:
        case INT_SERIAL:
        case INT_DMA0:
        case INT_DMA1:
        case INT_DMA2:
        case INT_DMA3:
        case INT_KEYPAD:
        case INT_GAMEPAK:
            REG_IE |= N2BIT(intr);
        break;

        case INT_COUNT:
        case INT_ALL:
        break;
    }

    REG_IME = 1;
}

void
gba_disable_interrupt(intr_type_t intr)
{
    REG_IME    = 0;

    switch (intr)
    {
        case INT_VBLANK:
            REG_DISPSTAT &= ~LCDC_VBL;
            REG_IE &= N2BIT(intr);
        break;

        case INT_HBLANK:
            REG_DISPSTAT &= ~LCDC_HBL;
            REG_IE &= N2BIT(intr);
        break;

        case INT_VCOUNT:
            REG_DISPSTAT &= ~LCDC_VCNT;
            REG_IE &= ~N2BIT(intr);
        break;

        case INT_TIMER0:
        case INT_TIMER1:
        case INT_TIMER2:
        case INT_TIMER3:
        case INT_SERIAL:
        case INT_DMA0:
        case INT_DMA1:
        case INT_DMA2:
        case INT_DMA3:
        case INT_KEYPAD:
        case INT_GAMEPAK:
            REG_IE &= ~N2BIT(intr);
        break;

        case INT_ALL:
            REG_IE = 0;
            REG_DISPSTAT &= ~(LCDC_VBL | LCDC_HBL | LCDC_VCNT);
        break;

        case INT_COUNT:
        break;
    }

    REG_IME = 1;
}
