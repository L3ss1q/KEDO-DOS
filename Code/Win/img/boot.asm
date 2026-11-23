; KEDO BIOS boot sector
; Relocates to 0000:0600, then loads 70 sectors of kernel data from
; disk sectors 2-18, followed by the next three track-bounded chunks.
; The filesystem snapshot is reserved at CHS 2/0/1 (LBA 72), outside this load.
bits 16
org 0x0600

boot_entry:
    ; BIOS entered at 0000:7C00. Copy this sector below the BIOS data area
    ; before loading the kernel over the original 0000:7C00 location.
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov si, 0x7C00
    mov di, 0x0600
    mov cx, 256
    rep movsw
    jmp 0x0000:relocated

relocated:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9F00
    sti
    cld
    mov [boot_drive], dl

    mov si, boot_message
    call print_string

    ; CHS 0/0/2, 17 sectors -> physical 0000:1000.
    mov ax, 0x0211
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x1000
    int 0x13
    jc disk_error

    ; CHS 0/1/1, 18 sectors -> physical 0000:3200.
    mov ax, 0x0212
    mov ch, 0
    mov cl, 1
    mov dh, 1
    mov dl, [boot_drive]
    mov bx, 0x3200
    int 0x13
    jc disk_error

    ; CHS 1/0/1, 18 sectors -> physical 0000:5600.
    mov ax, 0x0212
    mov ch, 1
    mov cl, 1
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, 0x5600
    int 0x13
    jc disk_error

    ; CHS 1/1/1, 17 sectors -> physical 0000:7A00.
    mov ax, 0x0211
    mov ch, 1
    mov cl, 1
    mov dh, 1
    mov dl, [boot_drive]
    mov bx, 0x7A00
    int 0x13
    jc disk_error

    mov dl, [boot_drive]
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
boot_message db 'KEDO loading...', 13, 10, 0
error_message db 'Disk read failed. Check image.', 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
