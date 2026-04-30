global keyboard_isr
global timer_isr
global mouse_isr
global exc_common_stub
extern keyboard_handler
extern timer_handler
extern mouse_handler
extern exception_handler
extern schedule

section .text


%macro EXC_NOERR 1
global exc%1
exc%1:
    push qword 0
    push qword %1
    jmp exc_common_stub
%endmacro

%macro EXC_ERR 1
global exc%1
exc%1:
    push qword %1
    jmp exc_common_stub
%endmacro

EXC_NOERR 0
EXC_NOERR 1
EXC_NOERR 2
EXC_NOERR 3
EXC_NOERR 4
EXC_NOERR 5
EXC_NOERR 6
EXC_NOERR 7
EXC_ERR   8
EXC_NOERR 9
EXC_ERR   10
EXC_ERR   11
EXC_ERR   12
EXC_ERR   13
EXC_ERR   14
EXC_NOERR 15
EXC_NOERR 16
EXC_ERR   17
EXC_NOERR 18
EXC_NOERR 19
EXC_NOERR 20
EXC_ERR   21
EXC_NOERR 22
EXC_NOERR 23
EXC_NOERR 24
EXC_NOERR 25
EXC_NOERR 26
EXC_NOERR 27
EXC_NOERR 28
EXC_NOERR 29
EXC_ERR   30
EXC_NOERR 31

exc_common_stub:
    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rax

    cld
    mov rdi, rsp
    call exception_handler

    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15
    add rsp, 16 ; vector and error code
    iretq

%macro ISR_STUB 2
global %1
%1:
    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rax

    cld
    call %2

    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15
    iretq
%endmacro

ISR_STUB keyboard_isr, keyboard_handler

global timer_isr
timer_isr:
    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rax

    cld
    mov rdi, rsp
    call schedule
    mov rsp, rax

    mov al, 0x20
    out 0x20, al

    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15
    iretq

ISR_STUB mouse_isr, mouse_handler

global gdt_flush
gdt_flush:
    ; RDI contains the pointer to the GDTR struct
    lgdt [rdi]

    ; Load Data Segments (Index 2 in GDT = 2 * 8 = 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload Code Segment (Index 1 in GDT = 1 * 8 = 0x08)
    ; We push the CS selector (0x08) and the address we want to jump to, then retfq
    push 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq
.reload_cs:
    
    ; Load the Task Register (TSS is at index 5 = 5 * 8 = 0x28)
    mov ax, 0x28
    ltr ax
    
    ret

; Mark stack as non-executable (suppress linker warning)
section .note.GNU-stack noalloc noexec nowrite progbits