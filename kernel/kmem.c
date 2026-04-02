#include "kmem.h"

/* -------------------------------------------------------------------------
 * Block header
 *
 * Placed immediately before every allocation.  The `next` pointer links
 * all blocks in address order, forming a simple linked list of all heap
 * regions (both free and allocated).
 * ---------------------------------------------------------------------- */
#define HEAP_MAGIC 0xDEAD1337u   /* Sanity sentinel in every block header */

typedef struct block_header {
    uint32_t             size;   /* Usable data bytes after this header   */
    uint32_t             free;   /* 1 = available, 0 = allocated          */
    uint32_t             magic;  /* Must equal HEAP_MAGIC; detects corruption */
    struct block_header *next;   /* Next block header in address order    */
} block_header_t;

/* -------------------------------------------------------------------------
 * Heap storage – a static 256 KB array in BSS (zeroed at start-up)
 * ---------------------------------------------------------------------- */
static uint8_t        heap_storage[HEAP_SIZE];
static block_header_t *heap_head = (block_header_t *)0;   /* initialised in kmem_init */

/* -------------------------------------------------------------------------
 * kmem_init – Build the initial single-block free list
 * ---------------------------------------------------------------------- */
void kmem_init(void)
{
    heap_head         = (block_header_t *)heap_storage;
    heap_head->size   = HEAP_SIZE - sizeof(block_header_t);
    heap_head->free   = 1;
    heap_head->magic  = HEAP_MAGIC;
    heap_head->next   = (block_header_t *)0;
}

/* -------------------------------------------------------------------------
 * kmalloc – First-fit allocation with optional block splitting
 * ---------------------------------------------------------------------- */
void *kmalloc(uint32_t size)
{
    if (size == 0) {
        return (void *)0;
    }

    /* Round up to a 4-byte boundary for natural alignment */
    size = (size + 3u) & ~3u;

    block_header_t *curr = heap_head;

    while (curr) {
        if (curr->free && curr->size >= size) {
            /*
             * Split the block only when there is enough room to create a
             * new header AND at least 4 bytes of usable space after it.
             */
            if (curr->size >= size + sizeof(block_header_t) + 4u) {
                block_header_t *split =
                    (block_header_t *)((uint8_t *)curr
                                      + sizeof(block_header_t)
                                      + size);
                split->size  = curr->size - size - sizeof(block_header_t);
                split->free  = 1;
                split->magic = HEAP_MAGIC;
                split->next  = curr->next;

                curr->next = split;
                curr->size = size;
            }

            curr->free = 0;
            /* Return a pointer to the data area just after the header */
            return (void *)((uint8_t *)curr + sizeof(block_header_t));
        }
        curr = curr->next;
    }

    return (void *)0;   /* Out of memory */
}

/* -------------------------------------------------------------------------
 * kfree – Release a block and coalesce adjacent free blocks
 * ---------------------------------------------------------------------- */
void kfree(void *ptr)
{
    if (!ptr) {
        return;
    }

    block_header_t *block =
        (block_header_t *)((uint8_t *)ptr - sizeof(block_header_t));

    /* Sanity check: ignore invalid or already-freed blocks */
    if (block->magic != HEAP_MAGIC || block->free) {
        return;
    }

    block->free = 1;

    /*
     * Walk the entire list and merge any two consecutive free blocks.
     * We repeat until no more merges are possible (handles chains of
     * three or more adjacent free blocks created by multiple frees).
     */
    block_header_t *curr = heap_head;
    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            /* Absorb the next block into curr */
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next  = curr->next->next;
            /* Do not advance curr – check again from this position */
        } else {
            curr = curr->next;
        }
    }
}

/* -------------------------------------------------------------------------
 * kmem_info – Walk the block list and sum used/free bytes
 * ---------------------------------------------------------------------- */
void kmem_info(uint32_t *out_used, uint32_t *out_free, uint32_t *out_total)
{
    *out_used  = 0;
    *out_free  = 0;
    *out_total = HEAP_SIZE;

    block_header_t *curr = heap_head;
    while (curr) {
        if (curr->free) {
            *out_free += curr->size;
        } else {
            *out_used += curr->size;
        }
        curr = curr->next;
    }
}
