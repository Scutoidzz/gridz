global keyboard_isr
global timer_isr
global mouse_isr
extern keyboard_handler
extern timer_handler
extern mouse_handler
extern generic_exception_handler

%macro ISR_NOERR 1
global isr%1:
isr%1: push 0
       push %1 
       jmp common_stub
%endmacro

common_stub:
    push rax 
    push rcx 
    push rdx 
    push rsi 
    push rdi
    push r8
    push r9
    push r10
    push r11
    mov rdi, rsp
    call generic_exception_handler
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    add rsp, 16
    iretq

%macro INTERRUPT_STUB 2
global %1
%1:
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    ; Alignment: (5 pushed by CPU + 9 pushed here) * 8 = 112 bytes. 
    ; 112 % 16 == 0. Aligned.
    
    call %2

    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax
    iretq
%endmacro

INTERRUPT_STUB keyboard_isr, keyboard_handler
INTERRUPT_STUB timer_isr, timer_handler
INTERRUPT_STUB mouse_isr, mouse_handler