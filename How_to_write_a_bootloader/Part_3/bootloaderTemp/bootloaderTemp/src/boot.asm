[BITS 16]
[ORG 0x7c00]

CODE_OFFSET equ 0x08
DATA_OFFSET equ 0x10
KERNEL_LOAD_SEG equ 0x1000   ; Segment to load the kernel (could be any suitable memory address)
KERNEL_LOAD_OFFSET equ 0x0000 ; Offset for kernel loading
KERNEL_START_ADDR equ 0x100000 ; Kernel's entry point address in protected mode (1MB)

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    ; Load Kernel
    mov bx, KERNEL_LOAD_SEG   ; Segment where kernel will be loaded
    mov dh, 0x00             ; Head
    mov dl, 0x80             ; First Hard Disk
    mov cl, 0x02             ; Start reading from sector 2
    mov ch, 0x00             ; Cylinder 0
    mov ah, 0x02             ; BIOS Read Sectors Function
    mov al, 8                ; Number of sectors to read (adjust based on your kernel size)
    int 0x13                 ; Call BIOS

    jc disk_read_error       ; Handle disk read error if CF is set

    ; Load GDT and enter Protected Mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp CODE_OFFSET:PModeMain

disk_read_error:
    hlt                     ; Halt if there was an error reading from disk

;GDT Definition
gdt_start:
    dd 0x0
    dd 0x0

    ; Code Segment Descriptor
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

    ; Data Segment Descriptor
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]

extern kernel_main
PModeMain:
    mov ax, DATA_OFFSET
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov gs, ax
    mov ebp, 0x9C00
    mov esp, ebp

    in al, 0x92
    or al, 2
    out 0x92, al

    ; Jump to kernel start address
    jmp CODE_OFFSET:KERNEL_START_ADDR

times 510 - ($ - $$) db 0
dw 0xAA55
