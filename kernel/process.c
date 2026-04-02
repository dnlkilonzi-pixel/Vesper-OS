#include "process.h"
#include "string.h"
#include "vga.h"

#define NULL ((void *)0)

/* -------------------------------------------------------------------------
 * Module-level state
 * ---------------------------------------------------------------------- */
process_t  *current_process                = NULL;
static process_t  process_table[MAX_PROCESSES];
static uint32_t   next_pid = 0;

/* -------------------------------------------------------------------------
 * Idle process – runs when every other process is blocked or zombie.
 * Uses HLT to yield the CPU to the interrupt controller between ticks.
 * ---------------------------------------------------------------------- */
static void idle_process_fn(void)
{
    while (1) {
        __asm__ volatile ("sti; hlt");
        process_yield();
    }
}

/* -------------------------------------------------------------------------
 * process_init – zero the table and create the idle process
 * ---------------------------------------------------------------------- */
void process_init(void)
{
    uint32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROC_UNUSED;
        process_table[i].pid   = 0;
    }
    next_pid = 0;

    /* Create idle process first (gets PID 1, table index 0) */
    process_create("idle", idle_process_fn);
}

/* -------------------------------------------------------------------------
 * process_create – allocate a slot and initialise a kernel thread
 *
 * Initial kernel-stack layout (from top downward, matching the
 * push order in process_yield's prologue):
 *
 *   [esp+0]  return address  → entry function
 *   [esp+4]  EBX  (0)
 *   [esp+8]  ESI  (0)
 *   [esp+12] EDI  (0)
 *   [esp+16] EBP  (0)
 *
 * When process_yield() first switches to this process it pops
 * EBP, EDI, ESI, EBX and then RET s into entry().
 * ---------------------------------------------------------------------- */
process_t *process_create(const char *name, void (*entry)(void))
{
    uint32_t i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            break;
        }
    }
    if (i == MAX_PROCESSES) {
        return NULL;   /* table full */
    }

    process_t *p = &process_table[i];

    p->pid   = ++next_pid;
    p->state = PROC_READY;
    strncpy(p->name, name, sizeof(p->name) - 1u);
    p->name[sizeof(p->name) - 1u] = '\0';

    /* Set up the initial stack frame */
    uint32_t *stk = (uint32_t *)(p->kstack + PROCESS_KSTACK_SIZE);
    *(--stk) = (uint32_t)entry;   /* return address = entry point */
    *(--stk) = 0u;                /* EBX */
    *(--stk) = 0u;                /* ESI */
    *(--stk) = 0u;                /* EDI */
    *(--stk) = 0u;                /* EBP */

    p->esp = (uint32_t)stk;

    return p;
}

/* -------------------------------------------------------------------------
 * process_exit – terminate current process and yield away
 * ---------------------------------------------------------------------- */
void process_exit(void)
{
    if (current_process) {
        current_process->state = PROC_ZOMBIE;
    }
    process_yield();
    while (1) {}   /* should never be reached */
}

/* -------------------------------------------------------------------------
 * process_block – suspend current process until woken
 * ---------------------------------------------------------------------- */
void process_block(void)
{
    if (current_process) {
        current_process->state = PROC_BLOCKED;
    }
    process_yield();
}

/* -------------------------------------------------------------------------
 * process_wake – make a BLOCKED process READY
 * ---------------------------------------------------------------------- */
void process_wake(process_t *p)
{
    if (p && p->state == PROC_BLOCKED) {
        p->state = PROC_READY;
    }
}

/* -------------------------------------------------------------------------
 * process_wake_all_blocked – wake every BLOCKED process
 * Called from IRQ context (e.g. keyboard data ready) to avoid a situation
 * where multiple processes waiting for input are missed.
 * ---------------------------------------------------------------------- */
void process_wake_all_blocked(void)
{
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_BLOCKED) {
            process_table[i].state = PROC_READY;
        }
    }
}

/* -------------------------------------------------------------------------
 * sched_next – round-robin: find next READY process after current
 * Called from process.asm immediately before the stack swap.
 * ---------------------------------------------------------------------- */
process_t *sched_next(void)
{
    if (!current_process) {
        /* No current process yet – return the first ready one */
        for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
            if (process_table[i].state == PROC_READY ||
                process_table[i].state == PROC_RUNNING) {
                return &process_table[i];
            }
        }
        return &process_table[0];
    }

    uint32_t curr_idx = (uint32_t)(current_process - process_table);

    /* Mark current as READY (if it was RUNNING) so it can be selected again */
    if (current_process->state == PROC_RUNNING) {
        current_process->state = PROC_READY;
    }

    for (uint32_t i = 1; i <= MAX_PROCESSES; i++) {
        uint32_t idx = (curr_idx + i) % MAX_PROCESSES;
        if (process_table[idx].state == PROC_READY) {
            process_table[idx].state = PROC_RUNNING;
            return &process_table[idx];
        }
    }

    /* Fallback: run idle (index 0 = first created process) */
    process_table[0].state = PROC_RUNNING;
    return &process_table[0];
}

/* -------------------------------------------------------------------------
 * process_print_all – 'ps' command helper
 * ---------------------------------------------------------------------- */
void process_print_all(void)
{
    static const char * const state_names[] = {
        "unused ", "ready  ", "running", "blocked", "zombie "
    };

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("PID  STATE    NAME\n");
    vga_puts("---  -------  ----------------\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        const process_t *p = &process_table[i];
        if (p->state == PROC_UNUSED) {
            continue;
        }
        vga_print_uint(p->pid);
        vga_puts("    ");
        vga_puts(state_names[p->state]);
        vga_puts("  ");
        vga_puts(p->name);
        vga_putchar('\n');
    }
}
