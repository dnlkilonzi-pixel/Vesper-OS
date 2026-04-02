#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/*
 * VESPER OS – Intel 8259A Programmable Interrupt Controller (PIC) driver
 *
 * The PC/AT has two cascaded 8259A PICs:
 *   Master PIC: handles IRQ 0–7   (default INT vectors 0x08–0x0F – clash with CPU exceptions!)
 *   Slave  PIC: handles IRQ 8–15  (default INT vectors 0x70–0x77)
 *
 * We remap them so IRQs use vectors that do not conflict with CPU exceptions:
 *   Master PIC → INT 32–39  (0x20–0x27)
 *   Slave  PIC → INT 40–47  (0x28–0x2F)
 */

/* I/O port addresses */
#define PIC1_CMD   0x20   /* Master PIC: command port  */
#define PIC1_DATA  0x21   /* Master PIC: data port     */
#define PIC2_CMD   0xA0   /* Slave  PIC: command port  */
#define PIC2_DATA  0xA1   /* Slave  PIC: data port     */

/* End-of-Interrupt signal */
#define PIC_EOI    0x20

/* Vector base offsets after remapping */
#define PIC1_OFFSET 32    /* Master IRQ 0-7  → INT 32-39 */
#define PIC2_OFFSET 40    /* Slave  IRQ 8-15 → INT 40-47 */

/*
 * pic_init – Remap both PICs to vectors 32-47 and mask all IRQ lines.
 *            Call this before loading the IDT and enabling interrupts.
 */
void pic_init(void);

/*
 * pic_send_eoi – Acknowledge the end of an interrupt for the given IRQ.
 *
 * Must be called at the end of every IRQ handler.  If the IRQ originates
 * from the slave PIC (irq >= 8), an EOI is sent to both slave and master.
 */
void pic_send_eoi(uint8_t irq);

/*
 * pic_unmask_irq – Enable (unmask) a single IRQ line.
 * pic_mask_irq   – Disable (mask) a single IRQ line.
 *
 * irq: 0-15, where 0-7 = master PIC, 8-15 = slave PIC.
 */
void pic_unmask_irq(uint8_t irq);
void pic_mask_irq(uint8_t irq);

#endif /* PIC_H */
