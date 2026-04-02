#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/*
 * VESPER OS – System call interface (INT 0x80)
 *
 * Calling convention (mirrors Linux i386 ABI):
 *   EAX = syscall number
 *   EBX = arg0, ECX = arg1, EDX = arg2
 *   Return value in EAX.
 *
 * Syscall numbers
 * ---------------
 *   0  SYS_EXIT    – terminate current process (EBX = exit code, ignored)
 *   1  SYS_WRITE   – write bytes to stdout
 *                    EBX = buffer address, ECX = length
 *                    returns bytes written
 *   2  SYS_GETPID  – return current process PID
 *   3  SYS_SLEEP   – sleep for EBX milliseconds
 */

#define SYS_EXIT    0u
#define SYS_WRITE   1u
#define SYS_GETPID  2u
#define SYS_SLEEP   3u

/*
 * syscall_init – install INT 0x80 in the IDT with DPL=3 so that user-mode
 *               code can invoke it without triggering a GPF.
 */
void syscall_init(void);

/*
 * syscall_handler – C dispatcher called by the INT 0x80 ASM stub.
 * @regs : saved register frame (same layout as isr.h registers_t)
 */
void syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx);

#endif /* SYSCALL_H */
