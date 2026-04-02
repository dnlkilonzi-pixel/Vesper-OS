#include "elf.h"
#include "string.h"

/* -------------------------------------------------------------------------
 * elf_load – load an ELF32 executable from a memory buffer
 * ---------------------------------------------------------------------- */
uint32_t elf_load(const void *data, uint32_t len)
{
    if (!data || len < sizeof(elf32_ehdr_t)) {
        return 0;
    }

    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)data;

    /* Validate ELF magic */
    if (eh->e_ident[0] != ELF_MAGIC0 ||
        eh->e_ident[1] != ELF_MAGIC1 ||
        eh->e_ident[2] != ELF_MAGIC2 ||
        eh->e_ident[3] != ELF_MAGIC3) {
        return 0;
    }

    /* Must be 32-bit, little-endian, executable, i386 */
    if (eh->e_ident[4] != ELF_CLASS32  ||
        eh->e_ident[5] != ELF_DATA2LSB ||
        eh->e_type      != ELF_ET_EXEC  ||
        eh->e_machine   != ELF_EM_386) {
        return 0;
    }

    if (eh->e_phoff == 0 || eh->e_phnum == 0) {
        return 0;   /* no program headers */
    }

    /* Bounds-check the program-header table */
    if (eh->e_phoff + (uint32_t)eh->e_phnum * sizeof(elf32_phdr_t) > len) {
        return 0;
    }

    const uint8_t *base = (const uint8_t *)data;

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const elf32_phdr_t *ph =
            (const elf32_phdr_t *)(base + eh->e_phoff +
                                   (uint32_t)i * eh->e_phentsize);

        if (ph->p_type != ELF_PT_LOAD) {
            continue;
        }
        if (ph->p_filesz == 0) {
            continue;
        }

        /* Bounds-check the segment data in the file */
        if (ph->p_offset + ph->p_filesz > len) {
            return 0;
        }

        /* Copy file bytes to virtual address */
        memcpy((void *)ph->p_vaddr,
               base + ph->p_offset,
               ph->p_filesz);

        /* Zero the BSS portion of the segment (memsz > filesz) */
        if (ph->p_memsz > ph->p_filesz) {
            memset((void *)(ph->p_vaddr + ph->p_filesz),
                   0,
                   ph->p_memsz - ph->p_filesz);
        }
    }

    return eh->e_entry;
}
