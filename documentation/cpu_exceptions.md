# CPU Exceptions in x86_64

When an exception occurs, the CPU invokes a handler based on the vector. In 64-bit mode, some exceptions push an error code onto the stack.

## Exception Table

| Vector | Name | Error Code? |
| :--- | :--- | :--- |
| 0 | Division Error (#DE) | No |
| 6 | Invalid Opcode (#UD) | No |
| 8 | Double Fault (#DF) | Yes (0) |
| 13 | General Protection Fault (#GP) | Yes |
| 14 | Page Fault (#PF) | Yes |

## ISR Stack Frame (with Error Code)
1. SS
2. RSP
3. RFLAGS
4. CS
5. RIP
6. Error Code <-- SP points here

## ISR Stack Frame (No Error Code)
1. SS
2. RSP
3. RFLAGS
4. CS
5. RIP <-- SP points here

## Implementation Notes
- **Register Preservation**: You must save ALL volatile registers (`rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8-r11`).
- **Alignment**: The stack must be 16-byte aligned before calling C functions.
- **IRETQ**: Use `iretq` to return from the exception.
