.section .iwram,"ax",%progbits
.extern	intr_table
.code 32

.global	_gba_intr_main
_gba_intr_main:
    mov  r3, #0x4000000        @ GBA_IOBASE
    ldr  r2, [r3,#0x200]       @ read REG_IE
    and  r1, r2, r2, lsr #16   @ r1 = IE & IF
    ldrh r2, [r3, #-8]
    orr  r2, r2, r1            @ mirrored at 0x03fffff8
    strh r2, [r3, #-8]
    add  r3, r3, #0x200
    ldr  r2, =intr_table

_gba_find_irq:
    ldr  r0, [r2, #4]
    cmp  r0,#0
    beq  _gba_missing_isr
    ands r0, r0, r1
    bne  _gba_isr_jump
    add  r2, r2, #8
    b    _gba_find_irq

_gba_missing_isr:
    strh r1, [r3, #2]
    mov  pc, lr

_gba_isr_jump:
    strh r0, [r3, #2]
    mov  r0, lr
    ldr  r0, [r2]
    bx   r0

.pool
.end
