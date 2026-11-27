.section .bss
.globl original_brk
.lcomm original_brk 8			# início da heap (nome usado no teste do prof)
.lcomm heap_top 8				# fim atual da heap

.section .text

.globl setup_brk
.globl dismiss_brk
.globl memory_alloc
.globl memory_free

setup_brk:
	movq $12, %rax                # brk
	movq $0, %rdi                 # brk(0) devolve o endereço atual
	syscall

	movq %rax, original_brk
	movq %rax, heap_top           # heap começa vazia
	ret

dismiss_brk:
	movq original_brk, %rdi         # volta o brk pro início
	movq $12, %rax
	syscall

	movq %rax, heap_top
	ret

new_block_at_end:
	cmpq $0, %rdi                 # tamanho grande demais
	jl new_block_at_end_fail

	movq %rdi, %r8                # r8 = tamanho pedido
	movq heap_top, %r9            # r9 = onde o bloco novo começa
	movq %r9, %r10
	addq $9, %r10
	addq %r8, %r10                # r10 = novo fim da heap

	movq $12, %rax
	movq %r10, %rdi
	syscall                       # muda rax, rcx e r11

	cmpq %r10, %rax               # se não deu o que pedimos, falhou
	jne new_block_at_end_fail

	movb $1, (%r9)                # em uso
	movq %r8, 1(%r9)              # tamanho
	movq %r10, heap_top
	movq %r9, %rax
	addq $9, %rax                 # devolve o endereço dos dados
	ret

new_block_at_end_fail:
	movq $0, %rax
	ret

# procura um bloco livre que caiba (rdi = tamanho, sil = estratégia)
# devolve em rax o início do registro (ou 0) e em r10 o tamanho dele
find_block:
	movq original_brk, %r8			# bloco atual
	movq $0, %rax					# melhor bloco até agora

find_block_loop:
	cmpq heap_top, %r8            # chegou no fim da heap
	jge find_block_end

	movq 1(%r8), %r9              # tamanho do bloco atual

	cmpb $0, (%r8)                # ocupado, pula
	jne find_block_next

	cmpq %rdi, %r9                # não cabe, pula
	jl find_block_next

	cmpq $0, %rax                 # primeiro que cabe
	je find_block_take

	cmpb $0, %sil                 # strategy é unsigned char, só o byte baixo vale
	je find_block_worst

find_block_best:
	cmpq %r10, %r9                # menor que o melhor até agora
	jl find_block_take
	jmp find_block_next

find_block_worst:
	cmpq %r10, %r9                # maior que o melhor até agora
	jg find_block_take

find_block_next:
	addq $9, %r8
	addq %r9, %r8                 # próximo = atual + 9 + tamanho
	jmp find_block_loop

find_block_take:
	movq %r8, %rax
	movq %r9, %r10
	jmp find_block_next

find_block_end:
	ret

memory_alloc:
	cmpq $0, %rdi                 # tamanho grande demais
	jl memory_alloc_fail

	call find_block
	cmpq $0, %rax
	jne memory_alloc_reuse

	call new_block_at_end         # nenhum bloco livre serve
	ret

memory_alloc_reuse:
	movq %r10, %rcx
	subq %rdi, %rcx               # rcx = sobra
	cmpq $10, %rcx                # precisa caber registro (9) + 1 byte
	jl memory_alloc_whole

	movq %rdi, 1(%rax)
	movb $1, (%rax)
	movq %rax, %rdx
	addq $9, %rdx
	addq %rdi, %rdx               # rdx = registro da sobra
	movb $0, (%rdx)
	subq $9, %rcx
	movq %rcx, 1(%rdx)            # sobra vira bloco livre
	addq $9, %rax
	ret

memory_alloc_whole:
	movb $1, (%rax)               # sobra pequena, usa o bloco todo
	addq $9, %rax
	ret

memory_alloc_fail:
	movq $0, %rax
	ret

memory_free:
	cmpq $0, %rdi
	je memory_free_fail

	movb $0, -9(%rdi)             # marca como livre
	movq $0, %rax
	ret

memory_free_fail:
	movq $-1, %rax
	ret
	