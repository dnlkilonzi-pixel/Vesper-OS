#ifndef KMEM_H
#define KMEM_H

#include <stdint.h>

/*
 * VESPER OS – Kernel Heap Allocator
 *
 * A minimal first-fit heap allocator backed by a 256 KB static array.
 * No dynamic memory is required from the OS (no page allocator needed).
 *
 * Every allocation is preceded by a small block_header that tracks its size
 * and whether it is free.  Adjacent free blocks are coalesced on kfree().
 *
 * Usage:
 *   kmem_init();          // call once during kernel startup
 *   void *p = kmalloc(n); // allocate n bytes; returns NULL if out of memory
 *   kfree(p);             // release a previously allocated block
 */

/* Total size of the kernel heap (256 KiB) */
#define HEAP_SIZE (256 * 1024)

/*
 * kmem_init – Initialise the heap.  Must be called before kmalloc/kfree.
 */
void kmem_init(void);

/*
 * kmalloc – Allocate at least `size` bytes from the kernel heap.
 *
 * Size is rounded up to the next multiple of 4 for alignment.
 * Returns a pointer to the usable memory, or NULL if no block large
 * enough is available (out of memory).
 */
void *kmalloc(uint32_t size);

/*
 * kfree – Release a block previously returned by kmalloc.
 *
 * Passing NULL is a no-op.  Adjacent free blocks are coalesced to reduce
 * heap fragmentation.
 */
void kfree(void *ptr);

/*
 * kmem_info – Report heap usage statistics.
 *
 * @out_used  : Set to the total number of bytes currently allocated.
 * @out_free  : Set to the total number of bytes available.
 * @out_total : Set to the total heap size in bytes.
 */
void kmem_info(uint32_t *out_used, uint32_t *out_free, uint32_t *out_total);

#endif /* KMEM_H */
