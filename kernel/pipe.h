#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

/*
 * VESPER OS – IPC Pipe subsystem  (Tier 3)
 *
 * Each pipe is a fixed-size ring buffer (PIPE_BUF_SIZE bytes).
 * Writers push data; readers block until data is available.
 *
 * Pipes are identified by a non-negative integer pipe_id (0 … MAX_PIPES-1).
 * Any process may read or write to a pipe it knows the id of.
 *
 * Blocking read:
 *   pipe_read() calls process_block() when the pipe is empty and loops
 *   after being woken.  The writer calls process_wake_all_blocked() after
 *   writing data to ensure waiting readers are scheduled.
 */

#define PIPE_BUF_SIZE  256u   /* bytes per pipe ring buffer  */
#define MAX_PIPES        8u   /* max simultaneously open pipes */

/* pipe_init – zero-initialise the pipe table (called from kernel_main) */
void pipe_init(void);

/*
 * pipe_alloc – allocate a new pipe.
 * Returns the pipe_id (≥ 0) on success, or -1 if the table is full.
 */
int pipe_alloc(void);

/*
 * pipe_write – write up to @len bytes from @data into pipe @id.
 * Returns the number of bytes written (may be < len if the buffer is full),
 * or -1 on error.
 */
int pipe_write(int id, const void *data, uint32_t len);

/*
 * pipe_read – read up to @len bytes from pipe @id into @buf.
 * Blocks (via process_block) if the pipe is empty.
 * Returns the number of bytes read, or -1 on error.
 */
int pipe_read(int id, void *buf, uint32_t len);

/*
 * pipe_close – free a pipe slot so it may be re-used.
 * Any blocked reader will be unblocked and receive -1.
 */
void pipe_close(int id);

#endif /* PIPE_H */
