; keyboard_asm.asm
; Assembly stub for the keyboard interrupt handler.

section .text
bits 32
global keyboard_isr
global default_isr
extern keyboard_handler

; Keyboard interrupt handler (IRQ1 = interrupt 0x21)
keyboard_isr:
    pusha
    call keyboard_handler
    popa
    iret

; Default handler for all other interrupts — just ignore and return
default_isr:
    iret
