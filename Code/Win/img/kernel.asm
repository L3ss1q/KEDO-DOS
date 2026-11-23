; KEDO BIOS kernel - 16-bit real mode
; Build: nasm -f bin kernel/kernel.asm -o kernel.bin
; Loaded at physical 0000:1000 by boot/boot.asm.
bits 16
org 0x1000

%define MAX_NODES 32
%define NAME_SIZE 16
%define DATA_SIZE 128

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7B00
    sti
    cld
    mov [boot_drive], dl
    mov [current_drive], dl

    call fs_init
    mov si, banner
    call puts
.shell:
    call fs_save
    mov si, prompt
    call puts
    mov di, line_buffer
    mov cx, 127
    call read_line
    call parse_line
    mov si, command
    mov di, cmd_kedo
    call streq
    jc .check_kedo_streak
    mov byte [kedo_streak], 0
    cmp byte [command], 0
    je .shell
    jmp .dispatch
.check_kedo_streak:
    ; The kedo command is handled by the normal dispatcher below.
.dispatch:

    mov si, command
    mov di, cmd_help
    call streq
    jc .help
    mov si, command
    mov di, cmd_ver
    call streq
    jc .ver
    mov si, command
    mov di, cmd_hello
    call streq
    jc .hello
    mov si, command
    mov di, cmd_cls
    call streq
    jc .cls
    mov si, command
    mov di, cmd_ls
    call streq
    jc .ls
    mov si, command
    mov di, cmd_tree
    call streq
    jc .tree
    mov si, command
    mov di, cmd_pwd
    call streq
    jc .pwd
    mov si, command
    mov di, cmd_mkdir
    call streq
    jc .mkdir
    mov si, command
    mov di, cmd_touch
    call streq
    jc .touch
    mov si, command
    mov di, cmd_write
    call streq
    jc .write
    mov si, command
    mov di, cmd_cat
    call streq
    jc .cat
    mov si, command
    mov di, cmd_rm
    call streq
    jc .rm
    mov si, command
    mov di, cmd_rmdir
    call streq
    jc .rmdir
    mov si, command
    mov di, cmd_cd
    call streq
    jc .cd
    mov si, command
    mov di, cmd_halt
    call streq
    jc .halt
    mov si, command
    mov di, cmd_exit
    call streq
    jc .halt
    mov si, command
    mov di, cmd_kedo
    call streq
    jc .kedo
    mov si, command
    mov di, cmd_panic
    call streq
    jc .panic
    mov si, unknown_message
    call puts
    jmp .shell

.help:
    mov si, help_text
    call puts
    jmp .shell
.ver:
    mov si, version_text
    call puts
    jmp .shell
.hello:
    mov si, hello_text
    call puts
    jmp .shell
.cls:
    mov ax, 0x0003
    int 0x10
    jmp .shell
.ls:
    call fs_list
    jmp .shell
.tree:
    call fs_tree
    jmp .shell
.pwd:
    call fs_pwd
    jmp .shell
.mkdir:
    mov byte [new_type], 1
    call fs_create
    jmp .shell
.touch:
    mov byte [new_type], 2
    call fs_create
    jmp .shell
.write:
    call fs_write
    jmp .shell
.cat:
    call fs_cat
    jmp .shell
.rm:
    mov byte [delete_type], 2
    call fs_delete
    jmp .shell
.rmdir:
    mov byte [delete_type], 1
    call fs_delete
    jmp .shell
.cd:
    call fs_cd
    jmp .shell
.kedo:
    inc byte [kedo_streak]
    cmp byte [kedo_streak], 5
    jb .shell
    mov byte [kedo_streak], 0
    mov si, kedo_message
    call puts
    jmp .shell
.panic:
    mov si, panic_prefix
    call puts
    mov si, arguments
    call puts
    mov si, panic_suffix
    call puts
.panic_halt:
    cli
    hlt
    jmp .panic_halt
.halt:
    mov si, halt_text
    call puts
.halt_loop:
    cli
    hlt
    jmp .halt_loop

; ---------------- Console ----------------
puts:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp puts
.done:
    ret

read_line:
    ; DI=destination, CX=max bytes excluding NUL
    xor bx, bx
.next:
    xor ah, ah
    int 0x16
    cmp al, 13
    je .enter
    cmp al, 8
    je .backspace
    cmp al, 32
    jb .next
    cmp bx, cx
    jae .next
    mov [di+bx], al
    inc bx
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .next
.backspace:
    cmp bx, 0
    je .next
    dec bx
    mov byte [di+bx], 0
    mov al, 8
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    jmp .next
.enter:
    mov byte [di+bx], 0
    mov si, newline
    call puts
    ret

; Parse first token into command, remainder into arguments.
parse_line:
    mov di, command
    mov si, line_buffer
    call skip_spaces
    mov cx, 31
.copy_command:
    lodsb
    test al, al
    jz .command_done
    cmp al, ' '
    je .command_done
    cmp al, 9
    je .command_done
    cmp cx, 0
    je .command_done
    stosb
    dec cx
    jmp .copy_command
.command_done:
    mov al, 0
    stosb
    ; If the command ended at NUL (no argument separator), do not read
    ; beyond the input buffer.
    cmp byte [si-1], 0
    je .no_args
    call skip_spaces
    mov di, arguments
.copy_args:
    lodsb
    stosb
    test al, al
    jnz .copy_args
    ret
.no_args:
    mov byte [arguments], 0
    ret

skip_spaces:
    cmp byte [si], ' '
    jne .check_tab
    inc si
    jmp skip_spaces
.check_tab:
    cmp byte [si], 9
    jne .done
    inc si
    jmp skip_spaces
.done:
    ret

; Compare DS:SI with DS:DI. CF=1 if equal.
streq:
.loop:
    mov al, [si]
    cmp al, [di]
    jne .no
    test al, al
    jz .yes
    inc si
    inc di
    jmp .loop
.yes:
    stc
    ret
.no:
    clc
    ret

; ---------------- Persistent filesystem ----------------
; Snapshot occupies CHS cylinder 2, head 0, sectors 1-10.
; The bootloader reserves this area and loads the kernel below it.
; Snapshot layout: 16-byte header + 4704-byte node tables + padding.

fs_init:
    mov byte [fs_disk_ok], 1
    call fs_disk_read
    jc .read_failed

    ; Validate signature, format version, payload size, and root node.
    mov si, fs_snapshot
    mov di, fs_magic
    mov cx, 8
    repe cmpsb
    jne .format
    cmp word [fs_snapshot+8], 1
    jne .format
    cmp word [fs_snapshot+10], 4704
    jne .format
    cmp byte [node_used], 1
    jne .format
    cmp byte [node_type], 1
    jne .format

    mov byte [current_dir], 0
    mov byte [fs_dirty], 0
    ret

.read_failed:
    mov byte [fs_disk_ok], 0
    call fs_format
    mov byte [fs_dirty], 0
    mov si, disk_read_warning
    call puts
    ret

.format:
    call fs_format
    ret

fs_format:
    mov di, node_used
    mov cx, 4704
    xor al, al
    rep stosb

    mov di, fs_snapshot
    mov si, fs_magic
    mov cx, 8
    rep movsb
    mov word [fs_snapshot+8], 1
    mov word [fs_snapshot+10], 4704
    mov dword [fs_snapshot+12], 0

    mov byte [node_used], 1
    mov byte [node_type], 1
    mov byte [node_parent], 0
    mov byte [node_name], '/'
    mov byte [node_name+1], 0
    mov byte [current_dir], 0
    mov byte [fs_dirty], 1
    ret

; BIOS reads exactly ten sectors into the in-memory snapshot buffer.
fs_disk_read:
    push ax
    push bx
    push cx
    push dx
    push es
    xor ax, ax
    mov es, ax
    mov bx, fs_snapshot
    mov ax, 0x020A
    mov ch, 2
    mov cl, 1
    xor dh, dh
    mov dl, [current_drive]
    int 0x13
    jc .failed
    mov byte [fs_io_error], 0
    jmp .finish
.failed:
    mov byte [fs_io_error], 1
.finish:
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    cmp byte [fs_io_error], 0
    jne .bad
    clc
    ret
.bad:
    stc
    ret

; BIOS writes the ten-sector snapshot to the currently selected drive.
fs_disk_write:
    push ax
    push bx
    push cx
    push dx
    push es
    xor ax, ax
    mov es, ax
    mov bx, fs_snapshot
    mov ax, 0x030A
    mov ch, 2
    mov cl, 1
    xor dh, dh
    mov dl, [current_drive]
    int 0x13
    jc .failed
    mov byte [fs_io_error], 0
    jmp .finish
.failed:
    mov byte [fs_io_error], 1
.finish:
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    cmp byte [fs_io_error], 0
    jne .bad
    clc
    ret
.bad:
    stc
    ret

; Save only after a successful mutation. A failed initial disk read disables
; writes, preventing a transient read error from overwriting an unknown disk.
fs_save:
    cmp byte [fs_disk_ok], 1
    jne .done
    cmp byte [fs_dirty], 1
    jne .done
    mov word [fs_snapshot+8], 1
    mov word [fs_snapshot+10], 4704
    call fs_disk_write
    jc .failed
    mov byte [fs_dirty], 0
    jmp .done
.failed:
    mov si, disk_write_warning
    call puts
.done:
    ret

; Extract first argument token into arg1, rest into arg2.
split_arguments:
    mov si, arguments
    mov di, arg1
    call skip_spaces
    mov cx, 31
.first:
    lodsb
    test al, al
    jz .first_end
    cmp al, ' '
    je .first_end
    cmp al, 9
    je .first_end
    cmp cx, 0
    je .first_end
    stosb
    dec cx
    jmp .first
.first_end:
    xor al, al
    stosb
    cmp byte [si-1], 0
    je .no_rest
    call skip_spaces
    mov di, arg2
.rest:
    lodsb
    stosb
    test al, al
    jnz .rest
    ret
.no_rest:
    mov byte [arg2], 0
    ret

; Find arg1 in current directory. AL=index or FFh.
fs_find:
    xor bx, bx
    mov bl, 1
.find_loop:
    cmp bl, MAX_NODES
    jae .not_found
    cmp byte [node_used+bx], 1
    jne .next
    mov al, [node_parent+bx]
    cmp al, [current_dir]
    jne .next
    mov si, arg1
    mov ax, bx
    mov dx, NAME_SIZE
    mul dx
    mov di, node_name
    add di, ax
    call streq
    jc .found
.next:
    inc bl
    jmp .find_loop
.found:
    mov al, bl
    ret
.not_found:
    mov al, 0xFF
    ret

; Create node from arg1 using new_type. Existing names are rejected.
fs_create:
    call split_arguments
    cmp byte [arg1], 0
    je .usage
    mov si, arg1
    xor cx, cx
.name_length:
    lodsb
    test al, al
    jz .name_ok
    inc cx
    cmp cx, NAME_SIZE-1
    ja .name_too_long
    jmp .name_length
.name_ok:
    call fs_find
    cmp al, 0xFF
    jne .exists
    xor bx, bx
    mov bl, 1
.slot:
    cmp bl, MAX_NODES
    jae .full
    cmp byte [node_used+bx], 0
    je .allocate
    inc bl
    jmp .slot
.allocate:
    mov byte [node_used+bx], 1
    mov al, [new_type]
    mov [node_type+bx], al
    mov al, [current_dir]
    mov [node_parent+bx], al
    ; Clear file-data slot as well, so a reused node cannot expose old data.
    push bx
    mov ax, bx
    mov dx, DATA_SIZE
    mul dx
    mov di, node_data
    add di, ax
    mov cx, DATA_SIZE
    xor al, al
    rep stosb
    pop bx
    ; Clear fixed name slot, then copy at most 15 characters.
    mov ax, bx
    mov dx, NAME_SIZE
    mul dx
    mov di, node_name
    add di, ax
    mov cx, NAME_SIZE
    xor al, al
    rep stosb
    mov si, arg1
    mov ax, bx
    mov dx, NAME_SIZE
    mul dx
    mov di, node_name
    add di, ax
    mov cx, NAME_SIZE-1
.copy_name:
    lodsb
    test al, al
    jz .created
    stosb
    loop .copy_name
.created:
    mov byte [fs_dirty], 1
    mov si, created_message
    call puts
    ret
.exists:
    mov si, exists_message
    call puts
    ret
.full:
    mov si, full_message
    call puts
    ret
.name_too_long:
    mov si, name_too_long_message
    call puts
    ret
.usage:
    mov si, usage_name
    call puts
    ret

fs_list:
    mov si, list_header
    call puts
    xor bx, bx
    mov bl, 1
.loop:
    cmp bl, MAX_NODES
    jae .done
    cmp byte [node_used+bx], 1
    jne .next
    mov al, [node_parent+bx]
    cmp al, [current_dir]
    jne .next
    mov [list_index], bl
    cmp byte [node_type+bx], 1
    jne .file
    mov si, dir_prefix
    call puts
    jmp .name
.file:
    mov si, file_prefix
    call puts
.name:
    xor ax, ax
    mov al, [list_index]
    mov dx, NAME_SIZE
    mul dx
    mov si, node_name
    add si, ax
    call puts
    cmp byte [node_type+bx], 1
    je .directory_size
    ; Count file bytes until NUL, bounded by DATA_SIZE-1.
    xor ax, ax
    mov al, [list_index]
    mov dx, DATA_SIZE
    mul dx
    mov si, node_data
    add si, ax
    xor cx, cx
.count_bytes:
    cmp cx, DATA_SIZE-1
    jae .show_file_size
    cmp byte [si], 0
    je .show_file_size
    inc si
    inc cx
    jmp .count_bytes
.show_file_size:
    mov ax, cx
    call print_u16
    mov si, bytes_suffix
    call puts
    jmp .line_end
.directory_size:
    mov si, dir_size_text
    call puts
.line_end:
    mov si, newline
    call puts
.next:
    inc bl
    jmp .loop
.done:
    ret

; Recursive tree display. Each stack frame stores its parent index
; and next candidate node, so nested directories do not overwrite callers.
fs_tree:
    mov si, tree_header
    call puts
    mov byte [tree_depth], 0
    xor al, al
    call fs_tree_walk
    ret

fs_tree_walk:
    push bp
    mov bp, sp
    sub sp, 4
    mov [ss:bp-2], al
    mov word [ss:bp-4], 1
.loop:
    mov bx, [ss:bp-4]
    cmp bx, MAX_NODES
    jae .done
    cmp byte [node_used+bx], 1
    jne .next
    mov al, [node_parent+bx]
    cmp al, [ss:bp-2]
    jne .next

    ; Indent according to recursion depth.
    xor cx, cx
    mov cl, [tree_depth]
.indent:
    jcxz .prefix
    mov al, ' '
    call putc
    loop .indent
.prefix:
    mov bx, [ss:bp-4]
    cmp byte [node_type+bx], 1
    jne .file_prefix
    mov si, dir_prefix
    call puts
    jmp .print_name
.file_prefix:
    mov si, file_prefix
    call puts
.print_name:
    mov ax, [ss:bp-4]
    mov dx, NAME_SIZE
    mul dx
    mov si, node_name
    add si, ax
    call puts
    mov si, newline
    call puts

    ; Recurse into directories, preserving this frame's scan index.
    mov bx, [ss:bp-4]
    cmp byte [node_type+bx], 1
    jne .next
    mov al, bl
    inc byte [tree_depth]
    call fs_tree_walk
    dec byte [tree_depth]
.next:
    inc word [ss:bp-4]
    jmp .loop
.done:
    mov sp, bp
    pop bp
    ret

; Print unsigned AX as decimal. Preserves BX and CX.
print_u16:
    push bx
    push cx
    xor cx, cx
    mov bx, 10
.convert:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz .convert
.emit:
    pop ax
    add al, '0'
    call putc
    loop .emit
    pop cx
    pop bx
    ret

; Print character in AL using BIOS teletype.
putc:
    push bx
    push cx
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    pop cx
    pop bx
    ret

fs_pwd:
    ; Collect ancestor indices from current directory to root, then emit
    ; them in reverse order to display the complete path.
    xor cx, cx
    mov al, [current_dir]
.collect:
    test al, al
    jz .emit
    cmp cx, MAX_NODES-1
    jae .emit
    xor ah, ah
    mov bx, cx
    mov [path_stack+bx], al
    inc cx
    xor bx, bx
    mov bl, al
    mov al, [node_parent+bx]
    jmp .collect
.emit:
    mov si, slash
    call puts
    test cx, cx
    jz .end
    dec cx
.emit_part:
    xor ax, ax
    mov bx, cx
    mov al, [path_stack+bx]
    mov dx, NAME_SIZE
    mul dx
    mov si, node_name
    add si, ax
    call puts
    cmp cx, 0
    je .end
    mov al, '/'
    call putc
    dec cx
    jmp .emit_part
.end:
    mov si, newline
    call puts
    ret
fs_cd:
    ; Recover safely if current_dir ever points at an unused/non-directory node.
    xor bx, bx
    mov bl, [current_dir]
    cmp bl, 0
    je .current_ok
    cmp byte [node_used+bx], 1
    jne .reset_current
    cmp byte [node_type+bx], 1
    jne .reset_current
    jmp .current_ok
.reset_current:
    mov byte [current_dir], 0
.current_ok:
    call split_arguments
    cmp byte [arg1], 0
    jne .lookup
    mov byte [current_dir], 0
    ret
.lookup:
    mov si, arg1
    mov di, dotdot
    call streq
    jc .parent
    mov si, arg1
    mov di, slash
    call streq
    jc .root
    call fs_find
    cmp al, 0xFF
    je .missing
    mov bl, al
    cmp byte [node_type+bx], 1
    jne .notdir
    mov [current_dir], al
    ret
.parent:
    xor bx, bx
    mov bl, [current_dir]
    mov al, [node_parent+bx]
    mov [current_dir], al
    ret
.root:
    mov byte [current_dir], 0
    ret
.missing:
    mov si, missing_message
    call puts
    ret
.notdir:
    mov si, notdir_message
    call puts
    ret

fs_write:
    call split_arguments
    cmp byte [arg1], 0
    je .usage
    call fs_find
    cmp al, 0xFF
    jne .existing
    mov byte [new_type], 2
    call fs_create
    call fs_find
    cmp al, 0xFF
    je .done
.existing:
    mov bl, al
    cmp byte [node_type+bx], 1
    je .isdir
    ; Clear 128-byte content buffer and copy arg2.
    mov ax, bx
    mov dx, DATA_SIZE
    mul dx
    mov di, node_data
    add di, ax
    mov cx, DATA_SIZE
    xor al, al
    rep stosb
    mov ax, bx
    mov dx, DATA_SIZE
    mul dx
    mov di, node_data
    add di, ax
    mov si, arg2
    mov cx, DATA_SIZE-1
.copy:
    lodsb
    test al, al
    jz .written
    stosb
    loop .copy
.written:
    mov byte [fs_dirty], 1
    mov si, written_message
    call puts
.done:
    ret
.isdir:
    mov si, isdir_message
    call puts
    ret
.usage:
    mov si, usage_write
    call puts
    ret

fs_cat:
    call split_arguments
    call fs_find
    cmp al, 0xFF
    je .missing
    mov bl, al
    cmp byte [node_type+bx], 1
    je .isdir
    mov ax, bx
    mov dx, DATA_SIZE
    mul dx
    mov si, node_data
    add si, ax
    call puts
    mov si, newline
    call puts
    ret
.missing:
    mov si, missing_message
    call puts
    ret
.isdir:
    mov si, isdir_message
    call puts
    ret

fs_delete:
    call split_arguments
    call fs_find
    cmp al, 0xFF
    je .missing
    mov [target_node], al
    mov bl, al
    mov al, [delete_type]
    cmp [node_type+bx], al
    jne .wrong_type

    ; deldir removes the entire subtree, not just empty directories.
    mov al, [target_node]
    call fs_delete_node
    mov byte [fs_dirty], 1
    mov si, deleted_message
    call puts
    ret
.missing:
    mov si, missing_message
    call puts
    ret
.wrong_type:
    mov si, wrong_type_message
    call puts
    ret

; Recursive post-order deletion. A stack frame retains the parent node
; and scan cursor while child subtrees are removed.
; Input: AL = node index to remove.
fs_delete_node:
    push bp
    mov bp, sp
    sub sp, 4
    mov [ss:bp-2], al
    mov word [ss:bp-4], 1
.child_scan:
    mov bx, [ss:bp-4]
    cmp bx, MAX_NODES
    jae .remove_self
    cmp byte [node_used+bx], 1
    jne .next_child
    mov al, [node_parent+bx]
    cmp al, [ss:bp-2]
    jne .next_child
    mov al, bl
    call fs_delete_node
.next_child:
    inc word [ss:bp-4]
    jmp .child_scan
.remove_self:
    mov bx, [ss:bp-2]
    mov byte [node_used+bx], 0
    mov sp, bp
    pop bp
    ret

; ---------------- Data ----------------
new_type db 0
current_drive db 0
boot_drive db 0
fs_disk_ok db 0
fs_dirty db 0
fs_io_error db 0
tree_depth db 0
fs_magic db 'KEDOFS01',0
path_stack times MAX_NODES db 0
banner db 13,10,'KEDO DOS BENCHMARK 2',13,10,'Recursive persistent filesystem. Type help.',13,10,0
prompt db 'KEDO> ',0
newline db 13,10,0
slash db '/',0
dotdot db '..',0
cmd_help db 'help',0
cmd_ver db 'ver',0
cmd_hello db 'hello',0
cmd_cls db 'clr',0
cmd_ls db 'ls',0
cmd_tree db 'tree',0
cmd_pwd db 'wai',0
cmd_mkdir db 'newdir',0
cmd_touch db 'blank',0
cmd_write db 'wri',0
cmd_cat db 'read',0
cmd_rm db 'disc',0
cmd_rmdir db 'deldir',0
cmd_cd db 'cdir',0
cmd_halt db 'hlt',0
cmd_exit db 'exit',0
cmd_panic db 'panic',0
cmd_kedo db 'kedo',0
help_text db 'help: displays this menu', 13, 10, 'ver: shows current version', 13,10,'hello: idfk',13,10, 'clr: clears the screen since you have been so disorganized',13,10, 'ls: lists entries in the current directory',13,10,'tree: recursively displays the directory tree',13,10,'wai: Where Am I command. prints current directory',13,10, 'cdir NAME|..|/: changes directory. make sure to remember where the f*** you are',13,10,'newdir NAME: makes a directory',13,10,'blank NAME: makes a blank file',13,10,'wri NAME TEXT: makes or changes a file',13,10,'read NAME: f***ing reads',13,10,'disc NAME: discombobulates a file (thus deleting it)',13,10,'deldir NAME: recursively deletes a directory and its contents.',13,10, 'hlt: halts',13,10,'exit: shuts down but you have to shutdown',13,10,'panic: kernel panics',13,10,'kedo: try it five times in a row',13,10,'Persistent filesystem: 31 entries, 15-char names, 127-char files. tree is recursive.',13,10,0
version_text db 'kedo dos Benchmark 2 (BIOS)',13,10,0
hello_text db 'Please just use your computer, I do not want to communicate with you socially for i am an introvert',13,10,0
unknown_message db 'Unknown command. Type help.',13,10,0
created_message db 'Created.',13,10,0
exists_message db 'That name already exists.',13,10,0
full_message db 'Filesystem full.',13,10,0
name_too_long_message db 'Name too long (maximum 15 characters).',13,10,0
usage_name db 'Usage: newdir NAME or blank NAME',13,10,0
usage_write db 'Usage: wri NAME TEXT',13,10,0
written_message db 'Written.',13,10,0
missing_message db 'Not found.',13,10,0
notdir_message db 'Not a directory.',13,10,0
isdir_message db 'Is a directory.',13,10,0
deleted_message db 'Deleted.',13,10,0
wrong_type_message db 'Wrong node type.',13,10,0
notempty_message db 'Directory not empty.',13,10,0
panic_prefix db 'KEDO PANIC [255]: ',0
panic_suffix db 13,10,'System halted.',13,10,0
halt_text db 'KEDO halted. Please turn off your computer, for the developer was too lazy to add an ACPI',13,10,0
list_header db 'Contents:',13,10,0
dir_prefix db '[DIR] ',0
file_prefix db '[FILE] ',0
dir_size_text db '  <DIR>',0
bytes_suffix db ' B',0
kedo_message db 'SHUT THE F*** UP',13,10,0
tree_header db 'Directory tree:',13,10,0
disk_read_warning db 'WARNING: disk read failed; using RAM-only filesystem.',13,10,0
disk_write_warning db 'WARNING: filesystem save failed; changes remain in RAM.',13,10,0

line_buffer times 128 db 0
command times 32 db 0
arguments times 96 db 0
arg1 times 32 db 0
arg2 times 96 db 0
delete_type db 0
target_node db 0
current_dir db 0
kedo_streak db 0
list_index db 0
fs_snapshot:
    db 'KEDOFS01'
    dw 1
    dw 4704
    dd 0
node_used times MAX_NODES db 0
node_type times MAX_NODES db 0
node_parent times MAX_NODES db 0
node_name times MAX_NODES*NAME_SIZE db 0
node_data times MAX_NODES*DATA_SIZE db 0
times (5120 - 16 - 4704) db 0
