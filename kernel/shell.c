#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "kmem.h"
#include "port_io.h"
#include "timer.h"
#include "pmm.h"
#include "process.h"
#include "ata.h"
#include "fs.h"
#include "elf.h"
#include "string.h"

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
    vga_puts("  help             Show this help message\n");
    vga_puts("  clear            Clear the screen\n");
    vga_puts("  echo <text>      Print text to the screen\n");
    vga_puts("  version          Show OS version information\n");
    vga_puts("  meminfo          Show heap + physical memory statistics\n");
    vga_puts("  uptime           Show system uptime\n");
    vga_puts("  ps               List running processes\n");
    vga_puts("  mkfs             Format the VesperFS partition\n");
    vga_puts("  ls               List files in VesperFS\n");
    vga_puts("  cat <file>       Print file contents\n");
    vga_puts("  write <f> <txt>  Write text to a file\n");
    vga_puts("  run <file>       Load and execute an ELF32 binary from disk\n");
    vga_puts("  halt             Halt the CPU\n");
    vga_puts("  reboot           Reboot the system\n");
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
    vga_puts("VESPER OS  v0.3.0\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Architecture   : x86 32-bit Protected Mode\n");
    vga_puts("VGA buffer     : 0xB8000\n");
    vga_puts("Kernel base    : 0x1000\n");
    vga_puts("Interrupt model: IDT + 8259A PIC (IRQ0-15 -> INT 32-47)\n");
    vga_puts("Keyboard       : PS/2 interrupt-driven (IRQ1)\n");
    vga_puts("Timer          : PIT 100 Hz (IRQ0)\n");
    vga_puts("Memory         : PMM bitmap + 256 KB heap + paging (0-8 MB)\n");
    vga_puts("Scheduler      : co-operative round-robin\n");
    vga_puts("Syscalls       : INT 0x80 (DPL=3)\n");
    vga_puts("Disk           : ATA PIO (primary master)\n");
    vga_puts("Filesystem     : VesperFS (LBA 129+)\n");
    vga_puts("ELF loader     : ELF32 static executables\n");
}

static void cmd_meminfo(void)
{
    uint32_t used, free_bytes, total;
    kmem_info(&used, &free_bytes, &total);

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Heap Memory:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  Total  : ");
    vga_print_uint(total / 1024u);
    vga_puts(" KB (");
    vga_print_uint(total);
    vga_puts(" bytes)\n");
    vga_puts("  Used   : ");
    vga_print_uint(used);
    vga_puts(" bytes\n");
    vga_puts("  Free   : ");
    vga_print_uint(free_bytes);
    vga_puts(" bytes\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("Physical Memory (PMM):\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("  Total  : ");
    vga_print_uint(pmm_total_frames() * 4u);
    vga_puts(" KB (");
    vga_print_uint(pmm_total_frames());
    vga_puts(" frames)\n");
    vga_puts("  Free   : ");
    vga_print_uint(pmm_free_frames() * 4u);
    vga_puts(" KB (");
    vga_print_uint(pmm_free_frames());
    vga_puts(" frames)\n");
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

static void cmd_uptime(void)
{
    uint32_t secs  = timer_get_uptime_sec();
    uint32_t mins  = secs / 60u;
    uint32_t hours = mins  / 60u;
    secs  %= 60u;
    mins  %= 60u;

    vga_puts("Uptime: ");
    vga_print_uint(hours);
    vga_puts("h ");
    vga_print_uint(mins);
    vga_puts("m ");
    vga_print_uint(secs);
    vga_puts("s  (");
    vga_print_uint(timer_get_ticks());
    vga_puts(" ticks at 100 Hz)\n");
}

static void cmd_ps(void)
{
    process_print_all();
}

static void cmd_mkfs(void)
{
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_puts("Formatting VesperFS at LBA 129... ");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    if (fs_format() == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("done.\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("FAILED (disk write error).\n");
    }
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

static void cmd_ls(void)
{
    if (!fs_init()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("No VesperFS found. Run 'mkfs' first.\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }
    fs_ls();
}

static void cmd_cat(const char *name)
{
    if (name[0] == '\0') {
        vga_puts("Usage: cat <filename>\n");
        return;
    }
    if (!fs_init()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("No VesperFS found. Run 'mkfs' first.\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    fs_file_t f;
    if (fs_open(&f, name) != 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("File not found: ");
        vga_puts(name);
        vga_putchar('\n');
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    /* Read and print 128 bytes at a time */
    char buf[129];
    uint32_t n;
    while ((n = fs_read(&f, buf, 128u)) > 0) {
        buf[n] = '\0';
        vga_puts(buf);
    }
    fs_close(&f);
    vga_putchar('\n');
}

/*
 * cmd_write – write a text argument directly to a named file.
 *
 * Usage: write <filename> <text...>
 *
 * The filename must not contain spaces.  Everything after the second
 * space-separated token is stored verbatim as the file content.
 */
static void cmd_write(const char *args)
{
    /* Extract filename (first token) */
    char fname[FS_NAME_MAX + 1];
    uint32_t i = 0;
    while (args[i] && args[i] != ' ' && i < FS_NAME_MAX) {
        fname[i] = args[i];
        i++;
    }
    fname[i] = '\0';

    if (fname[0] == '\0') {
        vga_puts("Usage: write <filename> <text>\n");
        return;
    }

    /* Skip spaces between name and content */
    while (args[i] == ' ') {
        i++;
    }

    const char *content = args + i;
    uint32_t    len     = strlen(content);

    if (!fs_init()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("No VesperFS. Run 'mkfs' first.\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    if (fs_write_new(fname, content, len) == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts("Written: ");
        vga_puts(fname);
        vga_puts(" (");
        vga_print_uint(len);
        vga_puts(" bytes)\n");
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("Write failed.\n");
    }
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/*
 * cmd_run – load an ELF32 file from VesperFS into memory and execute it
 *           as a new kernel thread.
 */

/* Scratch buffer for loading ELF data – static to avoid large stack use */
#define RUN_BUF_SIZE (64u * 1024u)   /* 64 KB – sufficient for small programs */
static uint8_t run_buf[RUN_BUF_SIZE];

static void cmd_run(const char *name)
{
    if (name[0] == '\0') {
        vga_puts("Usage: run <filename>\n");
        return;
    }
    if (!fs_init()) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("No VesperFS. Run 'mkfs' first.\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    fs_file_t f;
    if (fs_open(&f, name) != 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("File not found: ");
        vga_puts(name);
        vga_putchar('\n');
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    if (f.size > RUN_BUF_SIZE) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("File too large (max 64 KB).\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        fs_close(&f);
        return;
    }

    uint32_t nread = fs_read(&f, run_buf, f.size);
    fs_close(&f);

    uint32_t entry = elf_load(run_buf, nread);
    if (entry == 0) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("Not a valid ELF32 binary.\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    /* Cast the entry point to a function and create a kernel thread for it */
    void (*fn)(void) = (void (*)(void))entry;
    process_t *p = process_create(name, fn);
    if (!p) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("Process table full.\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("Spawned PID ");
    vga_print_uint(p->pid);
    vga_puts(": ");
    vga_puts(name);
    vga_putchar('\n');
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Yield once so the new process gets a chance to run */
    process_yield();
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
    } else if (str_eq(cmd, "uptime")) {
        cmd_uptime();
    } else if (str_eq(cmd, "ps")) {
        cmd_ps();
    } else if (str_eq(cmd, "mkfs")) {
        cmd_mkfs();
    } else if (str_eq(cmd, "ls")) {
        cmd_ls();
    } else if (str_startswith(cmd, "cat ")) {
        cmd_cat(cmd + 4);
    } else if (str_eq(cmd, "cat")) {
        vga_puts("Usage: cat <filename>\n");
    } else if (str_startswith(cmd, "write ")) {
        cmd_write(cmd + 6);
    } else if (str_eq(cmd, "write")) {
        vga_puts("Usage: write <filename> <text>\n");
    } else if (str_startswith(cmd, "run ")) {
        cmd_run(cmd + 4);
    } else if (str_eq(cmd, "run")) {
        vga_puts("Usage: run <filename>\n");
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
