; keyboard_asm.asm
; Assembly stub for the keyboard interrupt handler.
;
; When an interrupt fires, the CPU pushes registers and jumps here.
; We save all registers, call our C handler, restore, then return.

section .text
bits 32
global keyboard_isr
extern keyboard_handler

keyboard_isr:
    pusha               ; Save all general-purpose registers
    call keyboard_handler
    popa                ; Restore all registers
    iret                ; Return from interrupt (restores CS, EIP, EFLAGS)
