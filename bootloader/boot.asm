; =============================================================================
; VESPER OS - Stage 1 Bootloader
; =============================================================================
; A 512-byte BIOS boot sector that:
;   1. Prints "VESPER OS Loading..." via BIOS INT 10h
;   2. Loads the kernel (32 sectors = 16 KB) from disk to address 0x1000
;   3. Sets up a minimal GDT for flat 32-bit protected mode
;   4. Enables protected mode (CR0.PE) and far-jumps into 32-bit code
;   5. Sets up data/stack segments and jumps to the kernel entry at 0x1000
;
; Memory layout after boot:
;   0x0000-0x04FF  BIOS data / IVT
;   0x1000-0x4FFF  Kernel image (loaded here)
;   0x7C00-0x7DFF  This boot sector
;   0x90000        Kernel stack base (grows downward)
; =============================================================================

[BITS 16]
[ORG 0x7C00]

; Kernel load address and sector count
KERNEL_OFFSET  equ 0x1000   ; Physical address to load kernel into
KERNEL_SECTORS equ 128      ; sectors to load (128 × 512 = 64 KB)

; E820 memory-map storage (safe area below the boot sector)
; Layout: [0x0500] uint16_t count, [0x0502] e820_entry_t[32]
MMAP_ADDR      equ 0x0500
MMAP_MAX       equ 32

; =============================================================================
; Entry point
; =============================================================================
start:
    ; Normalise segment registers: all point to 0x0000
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x7C00          ; Stack grows downward from just below the boot sector

    ; Save the boot-drive number that BIOS passes in DL
    mov [BOOT_DRIVE], dl

    ; Print the loading banner
    mov si, MSG_LOADING
    call print_string

    ; Detect available RAM via BIOS INT 15h / E820
    call detect_memory

    ; Load the kernel from disk
    call load_kernel

    ; Transition to 32-bit protected mode
    call switch_to_pm

    ; Should never be reached
    jmp $

; =============================================================================
; print_string  –  Print a NUL-terminated string via BIOS INT 10h (TTY mode)
; Input:  SI = address of string
; =============================================================================
print_string:
    pusha
    mov ah, 0x0E            ; BIOS teletype output function
.loop:
    lodsb                   ; Load byte at [SI] into AL, advance SI
    test al, al             ; NUL terminator?
    jz   .done
    int  0x10               ; Print character in AL
    jmp  .loop
.done:
    popa
    ret

; =============================================================================
; load_kernel  –  Read KERNEL_SECTORS sectors (starting at CHS 0/0/2)
;                 into memory at KERNEL_OFFSET using BIOS INT 13h
; =============================================================================
load_kernel:
    mov si, MSG_LOAD_KERNEL
    call print_string

    xor bx, bx
    mov es, bx              ; ES = 0x0000
    mov bx, KERNEL_OFFSET   ; BX = 0x1000  →  ES:BX = 0x0000:0x1000 = 0x1000

    mov dh, KERNEL_SECTORS  ; Number of sectors to read
    mov dl, [BOOT_DRIVE]    ; Drive number saved earlier
    call disk_load
    ret

; =============================================================================
; detect_memory  –  Build an E820 memory map at MMAP_ADDR
;
; Result at 0x0500:
;   [0x0500] uint16_t count          – number of valid entries (0 if failed)
;   [0x0502] e820_entry_t[32]        – entries, each 20 bytes
; =============================================================================
detect_memory:
    pusha

    ; ES:DI → buffer for E820 entries  (skip 2-byte count header)
    xor  ax, ax
    mov  es, ax
    mov  di, MMAP_ADDR + 2

    xor  ebx, ebx            ; EBX=0 signals "first call"
    xor  bp,  bp             ; BP will count valid entries
    mov  edx, 0x534D4150     ; 'SMAP' – expected return signature

.e820_loop:
    mov  eax, 0xE820
    mov  ecx, 20             ; request 20-byte entries
    int  0x15
    jc   .e820_done          ; carry set = done or error

    cmp  eax, 0x534D4150     ; verify BIOS returned 'SMAP'
    jne  .e820_done

    cmp  cx, 20              ; did we get at least 20 bytes?
    jl   .e820_skip

    inc  bp                  ; count this entry
    add  di, 20              ; advance buffer pointer

    cmp  bp, MMAP_MAX        ; stop if table full
    jge  .e820_done

.e820_skip:
    test ebx, ebx            ; EBX=0 means this was the last entry
    jnz  .e820_loop

.e820_done:
    ; Store entry count as a 16-bit word at MMAP_ADDR
    mov  [MMAP_ADDR], bp

    popa
    ret

; =============================================================================
; disk_load  –  Read DH sectors from boot drive into ES:BX
; Input:  ES:BX = destination buffer
;         DH    = number of sectors
;         DL    = drive number
; =============================================================================
disk_load:
    push dx                 ; Preserve DH (sector count) for verification

    mov ah, 0x02            ; INT 13h function: read sectors
    mov al, dh              ; Number of sectors to read
    mov ch, 0x00            ; Cylinder 0
    mov cl, 0x02            ; Start from sector 2 (sector 1 = this boot sector)
    mov dh, 0x00            ; Head 0

    int 0x13                ; BIOS disk interrupt
    jc  .disk_error         ; Carry flag set = read error

    pop dx                  ; Restore DH = original sector count
    cmp al, dh              ; Were all requested sectors read?
    jne .sectors_error
    ret

.disk_error:
    mov si, MSG_DISK_ERROR
    call print_string
    jmp $                   ; Hang – cannot recover

.sectors_error:
    mov si, MSG_SECTORS_ERROR
    call print_string
    jmp $

; =============================================================================
; GDT (Global Descriptor Table) – flat 32-bit memory model
;
; Entry 0 – Null descriptor  (required by x86 spec)
; Entry 1 – Code segment     Base=0, Limit=4 GB, Ring 0, Execute/Read
; Entry 2 – Data segment     Base=0, Limit=4 GB, Ring 0, Read/Write
; =============================================================================
gdt_start:

gdt_null:
    dd 0x00000000           ; Null descriptor (8 bytes of zeros)
    dd 0x00000000

gdt_code:
    ; Limit[15:0]  = 0xFFFF
    dw 0xFFFF
    ; Base[15:0]   = 0x0000
    dw 0x0000
    ; Base[23:16]  = 0x00
    db 0x00
    ; Access byte: Present | DPL=0 | S=1 | Code | Readable
    ;   P=1  DPL=00  S=1  Type=1010  →  0x9A
    db 10011010b
    ; Flags + Limit[19:16]: G=1 (4 KB) | D/B=1 (32-bit) | Limit=0xF  →  0xCF
    db 11001111b
    ; Base[31:24]  = 0x00
    db 0x00

gdt_data:
    ; Same layout as code segment, but Type=0010 (data, read/write) → Access=0x92
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

; GDT descriptor (6 bytes): size (16-bit) + linear address (32-bit)
gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; Table size minus 1
    dd gdt_start                  ; Linear address of the table

; Segment selector constants (byte offset into GDT)
CODE_SEG equ gdt_code - gdt_start   ; = 0x08
DATA_SEG equ gdt_data - gdt_start   ; = 0x10

; =============================================================================
; switch_to_pm  –  Disable interrupts, load GDT, set CR0.PE, far-jump to 32-bit
; =============================================================================
switch_to_pm:
    cli                         ; Disable hardware interrupts
    lgdt [gdt_descriptor]       ; Load our GDT into GDTR

    mov eax, cr0
    or  eax, 0x1                ; Set the Protection Enable (PE) bit
    mov cr0, eax

    ; Far jump: flushes the prefetch queue and loads CS = CODE_SEG
    jmp CODE_SEG:init_pm

; =============================================================================
; 32-bit protected-mode initialisation  (executed after the far jump above)
; =============================================================================
[BITS 32]
init_pm:
    ; Point all data-segment registers at the flat data descriptor
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Set up the kernel stack (grows downward from 0x90000)
    mov ebp, 0x90000
    mov esp, ebp

    ; Jump to the kernel entry point loaded at KERNEL_OFFSET
    jmp KERNEL_OFFSET

; =============================================================================
; Data
; =============================================================================
BOOT_DRIVE        db 0
MSG_LOADING       db "VESPER OS Loading...", 0x0D, 0x0A, 0
MSG_LOAD_KERNEL   db "Loading kernel...", 0x0D, 0x0A, 0
MSG_DISK_ERROR    db "Disk read error!", 0x0D, 0x0A, 0
MSG_SECTORS_ERROR db "Sectors read mismatch!", 0x0D, 0x0A, 0

; =============================================================================
; Boot sector padding and magic signature
; =============================================================================
times 510 - ($ - $$) db 0  ; Pad to 510 bytes
dw 0xAA55                   ; BIOS boot signature (little-endian)
