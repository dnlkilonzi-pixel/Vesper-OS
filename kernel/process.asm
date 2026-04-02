; =============================================================================
; VESPER OS – process_yield  (software context switch)
; =============================================================================
;
; void process_yield(void)
;
; Save the calling process's callee-saved registers onto its kernel stack,
; ask the C scheduler for the next process, swap to its saved stack, restore
; its callee-saved registers, and return – which jumps to wherever that
; process last called process_yield (or, for a brand-new process, to its
; entry function).
;
; Stack frame convention (identical to the one set up by process_create):
;
;   Before save (current stack):    After save:
;     [esp-4]  EBP pushed               ← new esp saved in ->esp
;     [esp-8]  EDI pushed
;     [esp-12] ESI pushed
;     [esp-16] EBX pushed
;     ...                                (return address already there)
;
; Callee-saved registers (System V i386 ABI): EBX, ESI, EDI, EBP.
; EAX, ECX, EDX are caller-saved and need not be preserved here.
; =============================================================================

[BITS 32]

[EXTERN current_process]    ; process_t *current_process  (process.c)
[EXTERN sched_next]         ; process_t *sched_next(void) (process.c)

global process_yield

process_yield:
    ; --- save callee-saved registers onto the current stack ---
    push ebp
    push edi
    push esi
    push ebx

    ; --- save ESP into current_process->esp (offset 0) ---
    mov  eax, [current_process]
    test eax, eax
    jz   .no_save               ; current_process == NULL on first call
    mov  [eax], esp             ; current_process->esp = esp

.no_save:
    ; --- ask the C scheduler for the next process ---
    call sched_next             ; EAX = next process_t *

    ; --- update current_process and switch stacks ---
    mov  [current_process], eax
    mov  esp, [eax]             ; esp = next_process->esp

    ; --- restore callee-saved registers from new stack ---
    pop  ebx
    pop  esi
    pop  edi
    pop  ebp

    ret                         ; jumps to saved return address on new stack
