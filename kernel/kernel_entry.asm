; =============================================================================
; VESPER OS – Kernel Entry Point
; =============================================================================
; This is the FIRST code executed in 32-bit protected mode after the
; bootloader jumps to address 0x1000.
;
; Responsibilities:
;   - Provide the `kernel_entry` symbol that the linker places at 0x1000
;   - Zero the BSS section (the flat-binary loader does not do this)
;   - Call the C function kernel_main()
;   - Hang in an infinite HLT loop if kernel_main() ever returns
; =============================================================================

[BITS 32]

[EXTERN kernel_main]    ; Defined in kernel/kernel.c
[EXTERN _bss_start]     ; Exported by linker.ld
[EXTERN _bss_end]       ; Exported by linker.ld

global kernel_entry

kernel_entry:
    ; ---- Zero the BSS section -----------------------------------------------
    ; The bootloader copies a raw binary; zero-initialised statics would
    ; contain garbage on real hardware without this step.
    mov  edi, _bss_start
    mov  ecx, _bss_end
    sub  ecx, edi           ; byte count
    xor  eax, eax
    rep  stosb              ; fill with zeros

    ; ---- Call the C kernel --------------------------------------------------
    call kernel_main

    ; kernel_main() should never return, but halt safely if it does
.hang:
    hlt
    jmp .hang
