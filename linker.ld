/*
 * VESPER OS – Kernel Linker Script
 *
 * The bootloader loads the raw kernel binary starting at physical address
 * 0x1000 and jumps there in 32-bit protected mode.
 *
 * kernel_entry.o is listed first on the ld command line so its .text section
 * lands at 0x1000 – exactly where the bootloader jumps.
 */

ENTRY(kernel_entry)

SECTIONS
{
    /* Virtual (and load) address of the kernel */
    . = 0x1000;

    .text :
    {
        *(.text)        /* Code */
    }

    .rodata :
    {
        *(.rodata*)     /* Read-only data (string literals, const tables) */
    }

    .data :
    {
        *(.data)        /* Initialised read-write data */
    }

    .bss :
    {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;   /* Linker symbols used to zero BSS if needed */
    }
}
