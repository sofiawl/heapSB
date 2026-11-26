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
	movq $12, %rax                # brk
	movq $0, %rdi                 # brk(0) devolve o endereço atual
	syscall

	movq %rax, heap_start(%rip)
	movq %rax, heap_top(%rip)     # heap começa vazia
	ret

dismiss_brk:
	movq heap_start(%rip), %rdi   # volta o brk pro início
	movq $12, %rax
	syscall

	movq %rax, heap_top(%rip)
	ret

new_block_at_end:
	cmpq $0, %rdi                 # tamanho grande demais
	jl new_block_at_end_fail

	movq %rdi, %r8                # r8 = tamanho pedido
	movq heap_top(%rip), %r9      # r9 = onde o bloco novo começa
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
	movq %r10, heap_top(%rip)
	movq %r9, %rax
	addq $9, %rax                 # devolve o endereço dos dados
	ret

new_block_at_end_fail:
	movq $0, %rax
	ret

find_block:
	movq heap_start(%rip), %r8		# Atual = heap_start	
	movq $0, %rax					# Melhor = 0

find_block_loop:
	# parada: se atual >= heap_top, vai para find_block_end
	cmpq heap_top(%rip), %r8
	jge find_block_end

	movq 1(%r8), %r9		# R9 = tamanho do bloco

	# confere se está livre, se não, vai para find_block_next
	cmpb $0, (%r8)
	jne find_block_next

	# confere se cabe, se não, vai para find_block_next
	cmpq %rdi, %r9
	jl find_block_next

	# confere se é o primeiro, se sim, vai para find_block_take
	cmpq $0, %rax
	je find_block_take

	# verifica se é best ou worst na entrada
	cmpb $0, %sil		# Pega o último byte do rsi pois o resto pode ser lixo
	je find_block_worst

find_block_best:
	# se tamanho < tam_melhor, vai para find_block_take
	cmpq %r10, %r9
	jl find_block_take
	# senão, vai para find_block_next
	jmp find_block_next

find_block_worst:
	# se tamanho > tam_melhor, vai para find_block_take
	cmpq %r10, %r9
	jg find_block_take
	# senão, cai em find_block_next

find_block_next:
	addq $9, %r8
	addq %r9, %r8					# Atual = atual + 9 + tamanho
	jmp find_block_loop

find_block_take:
	movq %r8, %rax				# Melhor = atual
	movq %r9, %r10				# Tam_melhor = tamanho
	jmp find_block_next

find_block_end:
	ret

memory_alloc:
	call find_block
	testq %rax, %rax
	jz new_block_at_end

	movq %r10, %rcx
	subq %rdi, %rcx
	cmpq $10, %rcx			# sobra mínima: registro (9) + 1 byte
	jb memory_alloc_whole

	movq %rdi, 1(%rax)
	movb $1, (%rax)
	leaq 9(%rax, %rdi), %rdx
	movb $0, (%rdx)
	subq $9, %rcx
	movq %rcx, 1(%rdx)
	leaq 9(%rax), %rax
	ret

memory_alloc_whole:
	movb $1, (%rax)
	leaq 9(%rax), %rax
	ret

memory_free:
	movb $0, -9(%rdi)
	xorl %eax, %eax
	ret

.section .note.GNU-stack,"",@progbits
