; =============================================================================
; VESPER OS – process_yield  (software context switch, Tier 2)
; =============================================================================
;
; void process_yield(void)
;
; Save the calling process's callee-saved registers, ask the C scheduler for
; the next process, call process_before_switch() to update TSS.ESP0 and CR3,
; swap the kernel stack, restore callee-saved registers, and RET.
;
; struct process_t field offsets (MUST match process.h):
;   esp      = 0
;   pd_phys  = 4
;   state    = 8
;   pid      = 12
;   is_user  = 16
;   user_eip = 20
;   user_esp = 24
;   name     = 28
;   kstack   = 60
;
; =============================================================================

[BITS 32]

[EXTERN current_process]         ; process_t *current_process
[EXTERN sched_next]              ; process_t *sched_next(void)
[EXTERN process_before_switch]   ; void process_before_switch(process_t *)

; struct offsets
PROC_ESP      equ 0
PROC_USER_EIP equ 20
PROC_USER_ESP equ 24

global process_yield
global user_mode_enter

; =============================================================================
; process_yield – co-operative / preemptive context switch
; =============================================================================
process_yield:
    ; --- push callee-saved registers ---
    push ebp
    push edi
    push esi
    push ebx

    ; --- save current process's ESP ---
    mov  eax, [current_process]
    test eax, eax
    jz   .no_save
    mov  [eax + PROC_ESP], esp   ; current_process->esp = esp

.no_save:
    ; --- pick the next process (EAX = next process_t*) ---
    call sched_next

    ; --- keep next ptr in ESI (callee-saved, survives the next call) ---
    mov  esi, eax
    mov  [current_process], esi

    ; --- pre-switch housekeeping: TSS.ESP0 + CR3 ---
    push esi
    call process_before_switch   ; void process_before_switch(process_t *)
    add  esp, 4

    ; --- switch to the new process's kernel stack ---
    mov  esp, [esi + PROC_ESP]   ; esp = next_process->esp

    ; --- restore callee-saved registers from new stack ---
    pop  ebx
    pop  esi
    pop  edi
    pop  ebp

    ret   ; returns into the new process's execution context

; =============================================================================
; user_mode_enter – IRET trampoline for ring-3 processes
;
; Called via a RET from process_yield when a user-mode process is first
; scheduled.  Reads user_eip and user_esp from current_process, builds an
; IRET frame, switches to user-mode segment registers, and executes IRET.
;
; After IRET the CPU is at ring 3, executing user_eip with user_esp.
; =============================================================================
user_mode_enter:
    mov  eax, [current_process]

    ; Load user-mode data segment selectors (GDT_USER_DS | RPL=3 = 0x23)
    mov  ax, 0x23
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    ; Reload EAX (segment move clobbered the low 16 bits)
    mov  eax, [current_process]

    ; Build the IRET frame on the kernel stack:
    ;   [SS, user_ESP, EFLAGS, CS, user_EIP]
    push dword 0x23                        ; SS  = GDT_USER_DS | RPL=3
    push dword [eax + PROC_USER_ESP]       ; user-mode ESP
    pushfd
    or   dword [esp], 0x200                ; ensure IF=1 (interrupts enabled)
    push dword 0x1B                        ; CS  = GDT_USER_CS | RPL=3
    push dword [eax + PROC_USER_EIP]       ; user-mode EIP

    iret   ; privilege change: ring 0 → ring 3

