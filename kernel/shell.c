#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "kmem.h"
#include "port_io.h"

/* Maximum number of characters accepted per command line */
#define SHELL_BUF_SIZE  256

/* Shell prompt string */
#define SHELL_PROMPT    "vesper> "

/* -------------------------------------------------------------------------
 * Minimal string utilities  (no libc available)
 * ---------------------------------------------------------------------- */

static int str_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int str_startswith(const char *str, const char *prefix)
{
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++; prefix++;
    }
    return 1;
}

/* -------------------------------------------------------------------------
 * Built-in commands
 * ---------------------------------------------------------------------- */

static void cmd_help(void)
{
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("VESPER OS Shell  -  Available commands:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  help           Show this help message\n");
    vga_puts("  clear          Clear the screen\n");
    vga_puts("  echo <text>    Print text to the screen\n");
    vga_puts("  version        Show OS version information\n");
    vga_puts("  meminfo        Show heap memory statistics\n");
    vga_puts("  halt           Halt the CPU\n");
    vga_puts("  reboot         Reboot the system\n");
}

static void cmd_clear(void)
{
    vga_clear();
}

static void cmd_echo(const char *args)
{
    vga_puts(args);
    vga_putchar('\n');
}

static void cmd_version(void)
{
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("VESPER OS  v0.2.0\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Architecture   : x86 32-bit Protected Mode\n");
    vga_puts("VGA buffer     : 0xB8000\n");
    vga_puts("Kernel base    : 0x1000\n");
    vga_puts("Interrupt model: IDT + 8259A PIC (IRQ0-15 -> INT 32-47)\n");
    vga_puts("Keyboard       : PS/2 interrupt-driven (IRQ1)\n");
    vga_puts("Heap           : first-fit 256 KB\n");
}

static void cmd_meminfo(void)
{
    uint32_t used, free_bytes, total;
    kmem_info(&used, &free_bytes, &total);

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Heap Memory Information:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    vga_puts("  Total  : ");
    vga_print_uint(total / 1024);
    vga_puts(" KB (");
    vga_print_uint(total);
    vga_puts(" bytes)\n");

    vga_puts("  Used   : ");
    vga_print_uint(used);
    vga_puts(" bytes\n");

    vga_puts("  Free   : ");
    vga_print_uint(free_bytes);
    vga_puts(" bytes\n");
}

static void cmd_halt(void)
{
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("CPU halted. Reset to continue.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    __asm__ volatile ("cli; hlt");
    while (1) {}
}

static void cmd_reboot(void)
{
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Rebooting...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    /* Pulse the keyboard controller reset line (port 0x64 bit 0) */
    outb(0x64, 0xFE);
    /* If that fails, spin */
    while (1) {}
}

static void cmd_unknown(const char *cmd)
{
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("Unknown command: ");
    vga_puts(cmd);
    vga_putchar('\n');
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Type 'help' for a list of available commands.\n");
}

/* -------------------------------------------------------------------------
 * Input / dispatch helpers
 * ---------------------------------------------------------------------- */

static void read_line(char *buf, int max_len)
{
    int len = 0;

    while (len < max_len - 1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putchar('\n');
            break;
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                vga_putchar('\b');
            }
        } else {
            buf[len++] = c;
            vga_putchar(c);
        }
    }

    buf[len] = '\0';
}

static void execute_command(char *cmd)
{
    /* Skip leading spaces */
    while (*cmd == ' ') cmd++;

    if (*cmd == '\0') return;   /* blank line */

    if (str_eq(cmd, "help")) {
        cmd_help();
    } else if (str_eq(cmd, "clear")) {
        cmd_clear();
    } else if (str_startswith(cmd, "echo ")) {
        cmd_echo(cmd + 5);
    } else if (str_eq(cmd, "echo")) {
        vga_putchar('\n');
    } else if (str_eq(cmd, "version")) {
        cmd_version();
    } else if (str_eq(cmd, "meminfo")) {
        cmd_meminfo();
    } else if (str_eq(cmd, "halt")) {
        cmd_halt();
    } else if (str_eq(cmd, "reboot")) {
        cmd_reboot();
    } else {
        cmd_unknown(cmd);
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void shell_init(void)
{
    /* keyboard_init() is now called directly from kernel_main() before STI */
}

void shell_run(void)
{
    char buf[SHELL_BUF_SIZE];

    while (1) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts(SHELL_PROMPT);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        read_line(buf, SHELL_BUF_SIZE);
        execute_command(buf);
    }
}
