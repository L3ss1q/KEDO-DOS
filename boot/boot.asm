; KEDO DOS - Benchmark 1
; NASM: nasm -f bin boot.asm -o boot.bin
; Produces a 512-byte BIOS boot sector for legacy BIOS / x86 real mode.
; This is a self-contained B1 prototype: prompt, keyboard input, basic commands.
; It does NOT yet load or execute a separate C kernel.

bits 16
org 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    cld

    mov si, boot_message
    call print_string

main_loop:
    mov si, prompt
    call print_string
    mov di, command_buffer
    xor cx, cx

read_command:
    xor ah, ah
    int 0x16                ; BIOS keyboard: wait for key
    cmp al, 13              ; Enter?
    je command_done
    cmp al, 8               ; Backspace?
    je backspace
    cmp al, 32               ; Ignore control characters
    jb read_command
    cmp cx, 63
    jae read_command
    stosb
    inc cx
    mov ah, 0x0E
    mov bh, 0
    int 0x10                ; BIOS teletype output
    jmp read_command

backspace:
    cmp cx, 0
    je read_command
    dec di
    dec cx
    mov byte [di], 0
    mov al, 8
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    jmp read_command

command_done:
    mov byte [di], 0
    call newline

    mov si, command_buffer
    mov di, cmd_help
    call strcmp
    jc do_help

    mov si, command_buffer
    mov di, cmd_ver
    call strcmp
    jc do_ver

    mov si, command_buffer
    mov di, cmd_hello
    call strcmp
    jc do_hello

    mov si, command_buffer
    mov di, cmd_clear
    call strcmp
    jc do_clear

    mov si, command_buffer
    mov di, cmd_fr
    call strcmp
    jc do_fr

    cmp byte [command_buffer], 0
    je main_loop

    mov si, unknown_message
    call print_string
    jmp main_loop

do_help:
    mov si, help_message
    call print_string
    jmp main_loop

do_ver:
    mov si, version_message
    call print_string
    jmp main_loop

do_hello:
    mov si, hello_message
    call print_string
    jmp main_loop

do_clear:
    mov ax, 0x0003          ; BIOS text mode 80x25; clears screen
    int 0x10
    jmp main_loop

do_fr:
    mov si, goodbye_message
    call print_string
halt:
    cli
    hlt
    jmp halt

; strcmp: DS:SI and DS:DI, sets carry if equal.
strcmp:
.compare:
    mov al, [si]
    cmp al, [di]
    jne .not_equal
    test al, al
    jz .equal
    inc si
    inc di
    jmp .compare
.equal:
    stc
    ret
.not_equal:
    clc
    ret

print_string:
    lodsb
    test al, al
    jz .print_done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp print_string
.print_done:
    ret

newline:
    mov si, newline_text
    call print_string
    ret

boot_message db 13,10, 'KEDO DOS - Benchmark 1', 13,10
             db 'BIOS real-mode command prompt prototype.',13,10,0
prompt db 'KEDO> ',0
cmd_help db 'help',0
cmd_ver db 'ver',0
cmd_hello db 'hello',0
cmd_clear db 'cls',0
cmd_fr db 'fr',0
help_message db 'Commands: help, ver, hello, cls, fr',13,10,0
version_message db 'kedo dos version 2.12132034+2.12132034i',13,10,0
hello_message db 'Hello from KEDO Benchmark 1!',13,10,0
unknown_message db 'Unknown command. Type help.',13,10,0
goodbye_message db 'KEDO halted.',13,10,0
newline_text db 13,10,0

command_buffer times 64 db 0

times 510 - ($ - $$) db 0
dw 0xAA55
