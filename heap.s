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
    movq heap_start(%rip), %rdi   # Endereço inicial a restaurar
    movq $12, %rax               # Syscall brk
    syscall

    movq %rax, heap_top(%rip)    # Guarda o fim retornado
    ret

new_block_at_end:
    movq %rdi, %r8                # r8 = bytes solicitados
    movq heap_top(%rip), %r9      # r9 = antigo heap_top
    movq %r9, %r10
    addq $9, %r10                 # r10 = heap_top + 9
    jc new_block_at_end_fail      # Overflow: falha
    addq %r8, %r10                # r10 = novo fim (heap_top + 9 + bytes)
    jc new_block_at_end_fail      # Overflow: falha

    movq $12, %rax                # Syscall brk
    movq %r10, %rdi               # Novo fim solicitado
    syscall                       # Altera rax, rcx e r11 (r8-r10 preservados)

    cmpq %r10, %rax               # brk retornou o endereço pedido?
    jne new_block_at_end_fail

    movb $1, (%r9)                # Cabeçalho: bloco ocupado (offset 0)
    movq %r8, 1(%r9)              # Cabeçalho: tamanho (offset 1)
    movq %r10, heap_top(%rip)     # Atualiza o fim da heap
    leaq 9(%r9), %rax             # Retorna o endereço dos dados
    ret

new_block_at_end_fail:
    xorq %rax, %rax               # Falha: retorna 0
    ret

memory_alloc:
    # Provisório: ignora a estratégia (%rsi) e sempre cria um bloco no fim.
    # Versão final: find_block -> se achou, use_block -> senão new_block_at_end.
    jmp new_block_at_end          # bytes já está em %rdi; o ret dela volta ao chamador

memory_free:
    ret

.section .note.GNU-stack,"",@progbits
