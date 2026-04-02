#include "shell.h"
#include "vga.h"
#include "keyboard.h"

/* Maximum number of characters accepted per command line */
#define SHELL_BUF_SIZE  256

/* Shell prompt string */
#define SHELL_PROMPT    "vesper> "

/* -------------------------------------------------------------------------
 * Minimal string utilities  (no libc available)
 * ---------------------------------------------------------------------- */

/* Return 1 if the two NUL-terminated strings are identical */
static int str_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }
    return *a == *b;
}

/* Return 1 if str begins with the given prefix */
static int str_startswith(const char *str, const char *prefix)
{
    while (*prefix) {
        if (*str != *prefix)
            return 0;
        str++;
        prefix++;
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
    vga_puts("VESPER OS  v0.1.0\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_puts("Architecture : x86 32-bit Protected Mode\n");
    vga_puts("VGA buffer   : 0xB8000\n");
    vga_puts("Kernel base  : 0x1000\n");
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

/* Read one line of keyboard input into buf (at most max_len-1 chars + NUL).
 * Handles Backspace and echoes every character to the VGA console. */
static void read_line(char *buf, int max_len)
{
    int len = 0;

    while (len < max_len - 1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            /* Enter: terminate the line */
            vga_putchar('\n');
            break;
        } else if (c == '\b') {
            /* Backspace: remove the last character if any */
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

/* Parse and execute a single command line */
static void execute_command(char *cmd)
{
    /* Skip any leading spaces */
    while (*cmd == ' ')
        cmd++;

    /* Blank line – do nothing */
    if (*cmd == '\0')
        return;

    if (str_eq(cmd, "help")) {
        cmd_help();
    } else if (str_eq(cmd, "clear")) {
        cmd_clear();
    } else if (str_startswith(cmd, "echo ")) {
        /* Pass everything after "echo " as the argument */
        cmd_echo(cmd + 5);
    } else if (str_eq(cmd, "echo")) {
        /* echo with no argument prints an empty line */
        vga_putchar('\n');
    } else if (str_eq(cmd, "version")) {
        cmd_version();
    } else {
        cmd_unknown(cmd);
    }
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

void shell_init(void)
{
    keyboard_init();
}

void shell_run(void)
{
    char buf[SHELL_BUF_SIZE];

    while (1) {
        /* Print the coloured prompt */
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_puts(SHELL_PROMPT);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        /* Read and execute one command */
        read_line(buf, SHELL_BUF_SIZE);
        execute_command(buf);
    }
}
