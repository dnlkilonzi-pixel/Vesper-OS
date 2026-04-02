#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "isr.h"

/*
 * VESPER OS – System call interface (INT 0x80)  [Tier 3]
 *
 * Calling convention (mirrors Linux i386 ABI):
 *   EAX = syscall number
 *   EBX = arg0,  ECX = arg1,  EDX = arg2
 *   Return value in EAX (handler writes regs->eax).
 *
 * Syscall numbers
 * ---------------
 *   0  SYS_EXIT         – terminate current process
 *   1  SYS_WRITE        – write bytes to VGA stdout
 *                         EBX = buffer address, ECX = length
 *                         returns bytes written
 *   2  SYS_GETPID       – return current process PID in EAX
 *   3  SYS_SLEEP        – sleep for EBX milliseconds
 *   4  SYS_YIELD        – voluntarily yield the CPU
 *   5  SYS_KILL         – kill process EBX; returns 0/-1
 *   6  SYS_OPEN         – open file EBX (name ptr); returns fd or -1
 *   7  SYS_READ         – read ECX bytes from fd EBX into buf EDX
 *                         returns bytes read or -1
 *   8  SYS_CLOSE        – close fd EBX
 *   9  SYS_GETTIME      – return seconds since midnight (from RTC)
 *  10  SYS_PIPE_CREATE  – allocate a new IPC pipe; returns pipe_id or -1
 *  11  SYS_PIPE_WRITE   – write ECX bytes from buf EDX to pipe EBX
 *                         returns bytes written
 *  12  SYS_PIPE_READ    – read ECX bytes from pipe EBX into buf EDX
 *                         blocks until data available; returns bytes read
 */

#define SYS_EXIT         0u
#define SYS_WRITE        1u
#define SYS_GETPID       2u
#define SYS_SLEEP        3u
#define SYS_YIELD        4u
#define SYS_KILL         5u
#define SYS_OPEN         6u
#define SYS_READ         7u
#define SYS_CLOSE        8u
#define SYS_GETTIME      9u
#define SYS_PIPE_CREATE 10u
#define SYS_PIPE_WRITE  11u
#define SYS_PIPE_READ   12u

/* Install INT 0x80 in the IDT with DPL=3 */
void syscall_init(void);

/*
 * syscall_handler – C dispatcher called by the INT 0x80 ASM stub.
 * @regs : full saved register frame (same layout as registers_t).
 *         The handler may set regs->eax as the return value.
 */
void syscall_handler(registers_t *regs);

#endif /* SYSCALL_H */
