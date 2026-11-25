# Trabalho prático: heap

Sistema de gerenciamento de alocação dinâmica de memória em assembly AMD64.

## O que deve ter
 
- [ ] único `.zip` com formato: `login_dinf_1-login_dinf_2-login_dinf_3.zip`
- [ ] `.s` só com a API. Esse arquivo não tem `_start`
- [ ] `.s` ou `.c` com o procedimento principal e os testes
- [ ] makefile que liga com os testes
- [ ] relatório de uma página: estratégias de implementação



### Formato do bloco

Cada bloco na heap é um registro de gerência seguido dos dados:

| campo | tamanho | significado |
| --- | --- | --- |
| USO | 1 byte | 0 livre, 1 em uso |
| TAMANHO | 8 bytes | quantidade de bytes do bloco de dados |
| DADOS | TAMANHO bytes | memória entregue a quem chamou |

O ponteiro devolvido por `memory_alloc` aponta para DADOS, nove bytes depois do início do bloco. O tamanho fica no deslocamento 1, como o enunciado descreve.


### Estratégia e divisão

`strategy = 0` é worst-fit: entre os blocos livres que cabem, usa o maior
`strategy = 1` é best-fit: entre os blocos livres que cabem, usa o menor

Se o bloco livre for maior que o pedido, a sobra só vira outro bloco quando couber um registro novo e pelo menos 1 byte de dados. O registro tem 9 bytes, então a sobra precisa ter no mínimo 10 bytes. Sem essa sobra, devolve o bloco inteiro.

Se não houver bloco livre adequado, o bloco novo entra no final da heap.


## O que falta
- [ ] `memory_alloc` ignora `strategy` e sempre cria um bloco no fim. A busca best-fit e worst-fit existe, mas ainda não é usada, e a divisão da sobra não está feita
- [ ] `memory_free` ainda não marca o bloco
- [ ] `memory_free` ainda não marca o bloco
- [ ] `main.c` só chama `setup_brk` e `dismiss_brk`. Falta alocar, liberar e conferir
