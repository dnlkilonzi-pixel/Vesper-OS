#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

/*
 * VESPER OS – Co-operative process / task scheduler
 *
 * Processes are kernel threads that share the same address space.  Each
 * process has its own 8 KB kernel stack.  Scheduling is co-operative:
 * a process explicitly calls process_yield() to give up the CPU.
 *
 * keyboard_getchar() calls process_yield() while the input buffer is empty,
 * and the keyboard IRQ handler wakes blocked processes when data arrives.
 *
 * Context-switch implementation (process.asm):
 *   process_yield() pushes EBX, ESI, EDI, EBP onto the current stack,
 *   saves ESP to current_process->esp, calls sched_next() to pick the next
 *   ready process, loads its saved ESP, pops the callee-save registers, and
 *   returns – which jumps to wherever the new process last yielded from.
 *   For a newly created process the stack is pre-initialised so that the
 *   first switch drops straight into the entry function.
 */

#define MAX_PROCESSES       16u
#define PROCESS_KSTACK_SIZE 8192u   /* 8 KB kernel stack per process */

typedef enum {
    PROC_UNUSED  = 0,
    PROC_READY   = 1,
    PROC_RUNNING = 2,
    PROC_BLOCKED = 3,
    PROC_ZOMBIE  = 4,
} proc_state_t;

typedef struct process {
    /*
     * IMPORTANT: esp MUST remain the first field (offset 0).
     * process.asm accesses it with  mov [eax], esp  without a structure
     * offset, so moving it will break the context switch.
     */
    uint32_t      esp;
    proc_state_t  state;
    uint32_t      pid;
    char          name[32];
    uint8_t       kstack[PROCESS_KSTACK_SIZE];
} process_t;

/* Pointer to the currently executing process */
extern process_t *current_process;

/* Initialise the process table and create the idle process (PID 1) */
void process_init(void);

/*
 * Create a new kernel thread.
 * Returns a pointer to the process_t, or NULL if the table is full.
 */
process_t *process_create(const char *name, void (*entry)(void));

/* Terminate the calling process (marks it ZOMBIE and yields) */
void process_exit(void);

/*
 * Block the calling process and immediately yield.
 * The process will not be scheduled again until process_wake() is called.
 */
void process_block(void);

/* Make a BLOCKED process READY so it can be scheduled again */
void process_wake(process_t *p);

/*
 * Print a one-line summary of every non-UNUSED process to the VGA console
 * (used by the 'ps' shell command).
 */
void process_print_all(void);

/*
 * sched_next – pick the next READY or RUNNING process (round-robin).
 * Called from process.asm; must NOT be called directly.
 */
process_t *sched_next(void);

/*
 * process_yield – save context, switch to next process, return in new context.
 * Implemented in kernel/process.asm.
 */
void process_yield(void);

#endif /* PROCESS_H */
