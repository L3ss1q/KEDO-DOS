
; KEDO DOS - Minimal bootloader
; Assemble: nasm -f bin boot.asm -o boot.bin
; Loads 16 sectors starting at sector 2 to physical address 0x1000.
; Intended for a raw disk image booted through legacy BIOS.

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

    mov [boot_drive], dl

    mov si, boot_message
    call print_string

    ; Read 16 sectors starting at sector 2 into 0000:1000.
    mov ah, 0x02
    mov al, 16
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x1000
    int 0x13
    jc disk_error

    ; Transfer control to the loaded kernel.
    jmp 0x0000:0x1000

disk_error:
    mov si, error_message
    call print_string

halt:
    cli
    hlt
    jmp halt

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp print_string
.done:
    ret

boot_drive db 0
boot_message db 'KEDO DOS bootloader', 13, 10, 0
error_message db 'Disk read failed!', 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
