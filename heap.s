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
