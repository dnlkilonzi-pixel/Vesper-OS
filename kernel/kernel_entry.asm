; =============================================================================
; VESPER OS – Kernel Entry Point
; =============================================================================
; This is the FIRST code executed in 32-bit protected mode after the
; bootloader jumps to address 0x1000.
;
; Responsibilities:
;   - Provide the `kernel_entry` symbol that the linker places at 0x1000
;   - Call the C function kernel_main()
;   - Hang in an infinite HLT loop if kernel_main() ever returns
; =============================================================================

[BITS 32]

[EXTERN kernel_main]   ; Defined in kernel/kernel.c

global kernel_entry

kernel_entry:
    call kernel_main   ; Hand control to the C kernel

    ; kernel_main() should never return, but halt safely if it does
.hang:
    hlt
    jmp .hang
