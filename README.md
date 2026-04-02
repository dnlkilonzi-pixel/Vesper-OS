# Vesper-OS

A minimal educational operating system built from scratch using Assembly, C, and QEMU.

## Boot Flow

```
BIOS  →  Bootloader (ASM, real mode)
      →  32-bit Protected Mode
      →  Kernel (C)
      →  Shell (interactive)
```

## Project Structure

```
Vesper-OS/
├── bootloader/
│   └── boot.asm          # 512-byte NASM boot sector
├── kernel/
│   ├── kernel_entry.asm  # Protected-mode ASM stub (entry at 0x1000)
│   ├── kernel.c / .h     # Kernel initialisation & banner
│   ├── vga.c / .h        # VGA text-mode driver (buffer at 0xB8000)
│   ├── keyboard.c / .h   # PS/2 keyboard driver (port 0x60)
│   └── shell.c / .h      # Interactive command shell
├── linker.ld             # Kernel linker script (kernel base = 0x1000)
├── Makefile              # Build system
└── README.md
```

## Memory Layout

| Address range | Contents                        |
|---------------|---------------------------------|
| 0x0000–0x04FF | BIOS data area / IVT            |
| 0x1000–0x4FFF | Kernel image (loaded here)      |
| 0x7C00–0x7DFF | Bootloader (this boot sector)   |
| 0xB8000       | VGA text-mode buffer            |
| 0x90000       | Kernel stack base (grows ↓)     |

## Prerequisites

| Tool              | Purpose                      |
|-------------------|------------------------------|
| `nasm`            | Assemble bootloader & stubs  |
| `gcc` (with `-m32`) or `i686-elf-gcc` | Compile the kernel |
| `ld` / `i686-elf-ld` | Link kernel to flat binary |
| `qemu-system-i386` | Emulate and test            |
| `make`            | Drive the build              |

Install on Ubuntu/Debian:

```bash
sudo apt-get install nasm gcc-multilib qemu-system-x86 make
```

## Building

```bash
make
```

The disk image is written to `build/vesper.img`.

## Running

```bash
make run
```

Or launch QEMU manually:

```bash
qemu-system-i386 -drive format=raw,file=build/vesper.img,index=0,media=disk -m 32M
```

## Shell Commands

| Command        | Description                     |
|----------------|---------------------------------|
| `help`         | List available commands         |
| `clear`        | Clear the screen                |
| `echo <text>`  | Print text to the screen        |
| `version`      | Show OS version information     |

## Architecture Notes

* **Bootloader** (`boot.asm`): runs in 16-bit real mode; uses BIOS INT 13h CHS
  reads to load 32 sectors (16 KB) of kernel binary from disk sector 2 into
  physical address 0x1000; sets up a flat GDT and switches to 32-bit protected
  mode before jumping to the kernel.

* **Kernel entry** (`kernel_entry.asm`): first code executed in protected mode;
  simply calls `kernel_main()` in C.

* **VGA driver** (`vga.c`): writes directly to the memory-mapped VGA text buffer
  at 0xB8000 with support for 16 colours, newlines, backspace, and scrolling.

* **Keyboard driver** (`keyboard.c`): polls the PS/2 data port (0x60) and
  translates Scancode Set 1 make/break codes to ASCII, tracking Shift state.

* **Shell** (`shell.c`): a simple read-eval loop; no dynamic memory allocation.
