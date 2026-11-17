.section .bss
.balign 8

.globl heap_start
.globl heap_top

heap_start:
    .zero 8

heap_top:
    .zero 8

.section .text

.globl setup_brk
.globl dismiss_brk
.globl memory_alloc
.globl memory_free

setup_brk:
    movq $12, %rax                # Número da syscall brk
    xorq %rdi, %rdi               # consultar endereço atual
    syscall                      # Endereço retornado em rax

    movq %rax, heap_start(%rip)   # Guarda o início da região
    movq %rax, heap_top(%rip)     # fim igual ao início
    ret

dismiss_brk:
    ret

memory_alloc:
    ret

memory_free:
    ret

.section .note.GNU-stack,"",@progbits