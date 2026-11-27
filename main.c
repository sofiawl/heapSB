#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "heap.h"

static unsigned char bloco_uso(void *p)
{
    return *((unsigned char *)p - 9);
}

static unsigned long bloco_tamanho(void *p)
{
    unsigned long tamanho;

    memcpy(&tamanho, (unsigned char *)p - 8, sizeof tamanho);
    return tamanho;
}

int main(void)
{
    void *pequeno;
    void *grande;
    void *pior;
    void *melhor;
    void *sobra;
    void *novo;

    printf("Iniciando teste da heap.\n");
    setup_brk();

    pequeno = memory_alloc(20, 0);
    grande = memory_alloc(50, 0);
    assert(pequeno != NULL && grande != NULL);
    assert(bloco_uso(pequeno) == 1 && bloco_tamanho(pequeno) == 20);
    assert(bloco_uso(grande) == 1 && bloco_tamanho(grande) == 50);
    assert((char *)grande == (char *)pequeno + 29);

    assert(memory_free(pequeno) == 1);
    assert(memory_free(grande) == 1);
    assert(bloco_uso(pequeno) == 0 && bloco_tamanho(pequeno) == 20);
    assert(bloco_uso(grande) == 0 && bloco_tamanho(grande) == 50);

    pior = memory_alloc(10, 0);
    assert(pior == grande);
    assert(bloco_uso(pior) == 1 && bloco_tamanho(pior) == 10);
    sobra = (char *)grande + 19;
    assert(bloco_uso(sobra) == 0 && bloco_tamanho(sobra) == 31);

    melhor = memory_alloc(11, 1);
    assert(melhor == pequeno);
    assert(bloco_uso(melhor) == 1 && bloco_tamanho(melhor) == 20);

    novo = memory_alloc(100, 1);
    assert(novo != NULL && bloco_tamanho(novo) == 100);
    assert((char *)novo > (char *)sobra);

    dismiss_brk();
    return 0;
}