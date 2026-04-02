; =============================================================================
; VESPER OS – Interrupt / IRQ stubs
; =============================================================================
;
; Every interrupt/exception vector needs an entry-point stub that:
;   1. Pushes a dummy error code (for vectors where the CPU does NOT push one)
;      so the stack layout is identical for every vector.
;   2. Pushes the interrupt vector number.
;   3. Falls through into the single common_stub.
;
; common_stub then:
;   - Saves all GP registers (PUSHA) and segment registers.
;   - Loads the kernel data segment into DS/ES/FS/GS.
;   - Calls the C function  interrupt_handler(registers_t *regs).
;   - Restores everything and executes IRET.
; =============================================================================

[BITS 32]

[EXTERN interrupt_handler]   ; C dispatcher defined in isr.c
[EXTERN syscall_handler]     ; C syscall dispatcher defined in syscall.c

; =========================================================================
; Macros
; =========================================================================

; ISR stub for vectors where the CPU does NOT push an error code.
; We push dummy 0 so the frame is uniform.
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0    ; dummy error code
    push dword %1   ; interrupt vector number
    jmp  common_stub
%endmacro

; ISR stub for vectors where the CPU automatically pushes an error code.
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; CPU has already pushed the error code onto the stack
    push dword %1   ; interrupt vector number
    jmp  common_stub
%endmacro

; IRQ stub – IRQ n is remapped to INT (n + 32) by pic_init().
%macro IRQ 2
global irq%1
irq%1:
    push dword 0    ; dummy error code (IRQs have none)
    push dword %2   ; INT vector = IRQ number + 32
    jmp  common_stub
%endmacro

; =========================================================================
; CPU Exception stubs  (vectors 0 – 19)
;
; Vectors that push an error code automatically:
;   8 (Double Fault), 10–14 (TSS/Segment/Stack/GP/PF), 17 (Alignment Check)
; All others do not.
; =========================================================================

ISR_NOERRCODE  0   ; #DE  Divide-by-Zero
ISR_NOERRCODE  1   ; #DB  Debug
ISR_NOERRCODE  2   ;      Non-Maskable Interrupt
ISR_NOERRCODE  3   ; #BP  Breakpoint
ISR_NOERRCODE  4   ; #OF  Overflow
ISR_NOERRCODE  5   ; #BR  Bound Range Exceeded
ISR_NOERRCODE  6   ; #UD  Invalid Opcode
ISR_NOERRCODE  7   ; #NM  Device Not Available (no FPU)
ISR_ERRCODE    8   ; #DF  Double Fault          (error code = 0 always)
ISR_NOERRCODE  9   ;      Coprocessor Segment Overrun (obsolete, no errcode)
ISR_ERRCODE   10   ; #TS  Invalid TSS
ISR_ERRCODE   11   ; #NP  Segment Not Present
ISR_ERRCODE   12   ; #SS  Stack-Segment Fault
ISR_ERRCODE   13   ; #GP  General Protection Fault
ISR_ERRCODE   14   ; #PF  Page Fault
ISR_NOERRCODE 15   ;      Reserved
ISR_NOERRCODE 16   ; #MF  x87 FPU Floating-Point Error
ISR_ERRCODE   17   ; #AC  Alignment Check
ISR_NOERRCODE 18   ; #MC  Machine Check
ISR_NOERRCODE 19   ; #XM  SIMD Floating-Point Exception

; =========================================================================
; Hardware IRQ stubs  (IRQ 0–15 → INT vectors 32–47 after PIC remapping)
; =========================================================================

IRQ  0, 32   ; IRQ0  – Programmable Interval Timer
IRQ  1, 33   ; IRQ1  – PS/2 Keyboard
IRQ  2, 34   ; IRQ2  – Cascade (internal, slave PIC)
IRQ  3, 35   ; IRQ3  – COM2 / COM4 serial
IRQ  4, 36   ; IRQ4  – COM1 / COM3 serial
IRQ  5, 37   ; IRQ5  – LPT2 or sound card
IRQ  6, 38   ; IRQ6  – Floppy disk controller
IRQ  7, 39   ; IRQ7  – LPT1 / spurious
IRQ  8, 40   ; IRQ8  – CMOS real-time clock
IRQ  9, 41   ; IRQ9  – ACPI / free
IRQ 10, 42   ; IRQ10 – Free / PCI
IRQ 11, 43   ; IRQ11 – Free / PCI
IRQ 12, 44   ; IRQ12 – PS/2 Mouse
IRQ 13, 45   ; IRQ13 – FPU / coprocessor error
IRQ 14, 46   ; IRQ14 – Primary ATA hard disk
IRQ 15, 47   ; IRQ15 – Secondary ATA hard disk

; =========================================================================
; INT 0x80 – system-call gate (DPL=3, callable from ring 3)
;
; Calling convention (Linux i386 ABI):
;   EAX = syscall number
;   EBX = arg0,  ECX = arg1,  EDX = arg2
;
; We save the live registers that syscall_handler might clobber, call the
; C dispatcher with the three arguments, then restore and IRET.
; =========================================================================
global isr_syscall
isr_syscall:
    pushad              ; save all GP registers

    push edx            ; arg2
    push ecx            ; arg1
    push ebx            ; arg0
    push eax            ; syscall number
    call syscall_handler
    add  esp, 16        ; clean up four arguments

    popad               ; restore GP registers
    iret

; =========================================================================
; common_stub – unified register-save / restore frame
;
; On entry the stack contains (top = low address):
;   [ESP+0]  int_no
;   [ESP+4]  err_code  (dummy 0 or real CPU error code)
;   [ESP+8]  EIP       ─┐
;   [ESP+12] CS         │  pushed by the CPU
;   [ESP+16] EFLAGS    ─┘
; =========================================================================
common_stub:
    pusha               ; push EAX, ECX, EDX, EBX, (old)ESP, EBP, ESI, EDI

    ; Save segment registers
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment (selector 0x10 from our flat GDT)
    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    ; Pass a pointer to the register frame as the sole argument
    push esp
    call interrupt_handler
    add  esp, 4         ; cdecl: caller cleans up argument

    ; Restore segment registers
    pop  gs
    pop  fs
    pop  es
    pop  ds

    popa                ; restore GP registers
    add  esp, 8         ; discard int_no and err_code
    iret                ; restore EIP, CS, EFLAGS (and ESP/SS if ring change)
