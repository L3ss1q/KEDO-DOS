/*
 * KEDO Benchmark 1 - C-side command logic reference.
 *
 * IMPORTANT:
 * This file is NOT compiled into boot.img by NASM. NASM assembles assembly,
 * not C. The boot.asm file currently contains a self-contained B1 prompt so
 * you can boot/test the milestone without waiting for a 16-bit C toolchain.
 *
 * This C file documents the command behavior intended to move into a
 * freestanding 16-bit kernel once the compiler/linker/startup ABI are chosen.
 */
#include "kedoError.h"
typedef unsigned char u8;

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}

/* A future kernel should replace these with real BIOS-backed I/O wrappers. */
extern void kprint(const char *text);
extern void kreadline(char *buffer, unsigned int capacity);

void kedo_b1_command(const char *command) {
    if (streq(command, "help")) {
        kprint("Commands: help, ver, hello, cls, fr\r\n");
    } else if (streq(command, "ver")) {
        kprint("kedo dos version 2.12132034+2.12132034i\r\n");
    } else if (streq(command, "hello")) {
        kprint("Hello from KEDO Benchmark 1!\r\n");
    } else if (streq(command, "cls")) {
        /* Implement through a BIOS video wrapper in the kernel. */
        kprint("\r\n[screen clear not wired in this C reference]\r\n");
    } else if (streq(command, "fr")) {
        kprint("KEDO halted.\r\n");
        for (;;) { }
    } else if (*command) {
        kprint("Unknown command. Type help.\r\n");
    }
}
