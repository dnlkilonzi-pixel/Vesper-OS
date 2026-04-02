#include "pic.h"
#include "port_io.h"

/*
 * 8259A Initialisation Command Words (ICW)
 *
 * ICW1 (sent to CMD port):
 *   0x11 = initialise | cascade mode | ICW4 required | edge-triggered
 *
 * ICW2 (sent to DATA port):
 *   Vector base offset for the PIC.
 *
 * ICW3 for master (sent to DATA port):
 *   Bitmask of IRQ lines connected to a slave – bit 2 = slave at IRQ2.
 *
 * ICW3 for slave (sent to DATA port):
 *   Cascade identity (= IRQ line on master to which it is connected = 2).
 *
 * ICW4 (sent to DATA port):
 *   0x01 = 8086/88 mode (not MCS-80/85).
 */

void pic_init(void)
{
    /* Save existing IRQ masks (we will replace them with "all masked") */

    /* ICW1 – start initialisation sequence in cascade mode */
    outb(PIC1_CMD,  0x11); io_wait();
    outb(PIC2_CMD,  0x11); io_wait();

    /* ICW2 – set vector base offsets */
    outb(PIC1_DATA, PIC1_OFFSET); io_wait();   /* master → INT 32 */
    outb(PIC2_DATA, PIC2_OFFSET); io_wait();   /* slave  → INT 40 */

    /* ICW3 – cascade wiring */
    outb(PIC1_DATA, 0x04); io_wait();   /* master: slave connected at IRQ2 */
    outb(PIC2_DATA, 0x02); io_wait();   /* slave:  cascade identity = 2    */

    /* ICW4 – 8086 mode */
    outb(PIC1_DATA, 0x01); io_wait();
    outb(PIC2_DATA, 0x01); io_wait();

    /* OCW1 – mask all IRQ lines (drivers unmask their own IRQ as needed) */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        /* IRQ from slave PIC – acknowledge slave first, then master */
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port  = PIC2_DATA;
        irq  -= 8;
        /* Also unmask the master's cascade line (IRQ2) */
        uint8_t master = inb(PIC1_DATA);
        outb(PIC1_DATA, master & ~(1u << 2));
    }

    value = inb(port);
    outb(port, value & ~(uint8_t)(1u << irq));
}

void pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port);
    outb(port, value | (uint8_t)(1u << irq));
}
