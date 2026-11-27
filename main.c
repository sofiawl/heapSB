#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include "heap.h"

extern unsigned long original_brk;
#define heap_start original_brk
#define heap_top ((unsigned long)syscall(SYS_brk, 0))

// auxiliares

static unsigned char uso(void *p)
{
    return *((unsigned char *)p - 9);
}

static unsigned long tamanho(void *p)
{
    unsigned long t;
    memcpy(&t, (unsigned char *)p - 8, sizeof t);
    return t;
}

// ponteiro de dados do proximo bloco
static void *proximo(void *p)
{
    return (char *)p + tamanho(p) + 9;
}

static unsigned long ocupado(void)
{
    return heap_top - heap_start;
}

// mapa da heap, so aparece com ./testes -v
static int verbose = 0;

static void mostra_heap(const char *quando)
{
    unsigned long atual = heap_start;
    unsigned long fim = heap_top;
    int n = 0;

    printf("   %s:\n", quando);
    while (atual < fim) {
        unsigned char u = *(unsigned char *)atual;
        unsigned long t;
        memcpy(&t, (void *)(atual + 1), 8);
        if (n < 12)
            printf("     bloco %d: inicio +%lu, %s, tam %lu\n",
                   n, atual - heap_start, u ? "usado" : "livre", t);
        atual += 9 + t;
        n++;
    }
    if (n > 12)
        printf("     ... mais %d blocos\n", n - 12);
    if (n == 0)
        printf("     vazia\n");
    printf("     topo +%lu\n", fim - heap_start);
}

#define ANTES()  if (verbose) mostra_heap("antes")
#define DEPOIS() if (verbose) mostra_heap("depois")

// setup e alloc no fim da heap
static void t_setup_heap_vazia(void)
{
    setup_brk();
    DEPOIS();
    assert(heap_start != 0);
    assert(heap_top == heap_start);
    dismiss_brk();
}

static void t_primeira_alocacao(void)
{
    setup_brk();
    ANTES();
    void *p = memory_alloc(16, 0);
    DEPOIS();
    assert(p != NULL);
    assert((unsigned long)p == heap_start + 9);
    assert(uso(p) == 1 && tamanho(p) == 16);
    assert(ocupado() == 9 + 16);
    dismiss_brk();
}

static void t_alocacoes_contiguas(void)
{
    setup_brk();
    ANTES();
    void *a = memory_alloc(5, 0);
    void *b = memory_alloc(30, 1);
    void *c = memory_alloc(1, 0);
    DEPOIS();
    assert(b == proximo(a));
    assert(c == proximo(b));
    assert(ocupado() == (9 + 5) + (9 + 30) + (9 + 1));
    dismiss_brk();
}

static void t_alocacao_tamanho_zero(void)
{
    setup_brk();
    ANTES();
    void *a = memory_alloc(0, 0);
    void *b = memory_alloc(8, 0);
    DEPOIS();
    assert(a != NULL && uso(a) == 1 && tamanho(a) == 0);
    assert(b == proximo(a));          // só o cabeçalho foi usado
    dismiss_brk();
}

// memory free

static void t_free_marca_livre(void)
{
    setup_brk();
    void *p = memory_alloc(40, 0);
    unsigned long topo = heap_top;
    ANTES();
    assert(memory_free(p) == 1);
    DEPOIS();
    assert(uso(p) == 0);
    assert(tamanho(p) == 40);         // tamanho não muda
    assert(heap_top == topo);         // heap não encolhe
    dismiss_brk();
}

static void t_free_nao_afeta_vizinhos(void)
{
    setup_brk();
    void *a = memory_alloc(10, 0);
    void *b = memory_alloc(10, 0);
    void *c = memory_alloc(10, 0);
    ANTES();
    memory_free(b);
    DEPOIS();
    assert(uso(a) == 1 && uso(b) == 0 && uso(c) == 1);
    assert(tamanho(a) == 10 && tamanho(c) == 10);
    dismiss_brk();
}

static void t_free_fora_da_heap(void)
{
    setup_brk();
    void *p = memory_alloc(10, 0);
    int local;
    ANTES();
    assert(memory_free(NULL) == 0);
    assert(memory_free(&local) == 0);                 // pilha
    assert(memory_free((void *)heap_start) == 0);     // antes do 1o dado
    assert(memory_free((char *)heap_top + 100) == 0); // depois do fim
    DEPOIS();
    assert(uso(p) == 1);                              // nada mudou
    dismiss_brk();
}

static void t_free_varios_blocos(void)
{
    setup_brk();
    void *a = memory_alloc(10, 0);
    void *b = memory_alloc(20, 0);
    void *c = memory_alloc(30, 0);
    ANTES();
    assert(memory_free(c) == 1);   // ult bloco
    assert(memory_free(a) == 1);   // primeiro bloco
    assert(memory_free(b) == 1);   // do meio
    DEPOIS();
    assert(uso(a) == 0 && uso(b) == 0 && uso(c) == 0);
    dismiss_brk();
}

static void t_free_bloco_zero_no_fim(void)
{
    setup_brk();
    memory_alloc(10, 0);
    void *z = memory_alloc(0, 0);  // bloco vazio, ultimo da heap
    ANTES();
    assert(memory_free(z) == 1);
    DEPOIS();
    assert(uso(z) == 0);
    dismiss_brk();
}

// reuso e div de blocos

static void t_reuso_tamanho_exato(void)
{
    setup_brk();
    void *a = memory_alloc(25, 0);
    memory_alloc(5, 0);               // impede que a seja o ultimo
    memory_free(a);
    unsigned long topo = heap_top;
    ANTES();
    void *b = memory_alloc(25, 0);
    DEPOIS();
    assert(b == a && uso(b) == 1 && tamanho(b) == 25);
    assert(heap_top == topo);         // n cresceu
    dismiss_brk();
}

static void t_divide_bloco(void)
{
    setup_brk();
    void *a = memory_alloc(50, 0);
    memory_alloc(5, 0);
    memory_free(a);
    ANTES();
    void *b = memory_alloc(20, 0);
    DEPOIS();
    assert(b == a && tamanho(b) == 20 && uso(b) == 1);
    void *s = proximo(b);
    assert(uso(s) == 0 && tamanho(s) == 50 - 20 - 9);
    dismiss_brk();
}

// sobra = 10 (cabeçalho + 1 byte): ent deve dividir
static void t_divide_limite_10(void)
{
    setup_brk();
    void *a = memory_alloc(20, 0);
    memory_alloc(5, 0);
    memory_free(a);
    ANTES();
    void *b = memory_alloc(10, 0);
    DEPOIS();
    assert(b == a && tamanho(b) == 10);
    void *s = proximo(b);
    assert(uso(s) == 0 && tamanho(s) == 1);
    dismiss_brk();
}

// sobra = 9 (só caberia o cabeçalho): ent N deve dividir
static void t_nao_divide_limite_9(void)
{
    setup_brk();
    void *a = memory_alloc(20, 0);
    void *z = memory_alloc(5, 0);
    memory_free(a);
    ANTES();
    void *b = memory_alloc(11, 0);
    DEPOIS();
    assert(b == a && tamanho(b) == 20);   // bloco inteiro
    assert(proximo(b) == z);
    dismiss_brk();
}

static void t_sobra_reutilizavel(void)
{
    setup_brk();
    void *a = memory_alloc(60, 0);
    memory_alloc(5, 0);
    memory_free(a);
    void *b = memory_alloc(20, 0);
    void *s = proximo(b);             // sobra de 31 bytes
    unsigned long topo = heap_top;
    ANTES();
    void *c = memory_alloc(31, 0);
    DEPOIS();
    assert(c == s && uso(c) == 1 && tamanho(c) == 31);
    assert(heap_top == topo);
    dismiss_brk();
}

static void t_bloco_ocupado_ignorado(void)
{
    setup_brk();
    memory_alloc(100, 0);             // grande, mas ocupado
    void *p = memory_alloc(8, 0);
    memory_alloc(5, 0);
    memory_free(p);
    ANTES();
    void *q = memory_alloc(50, 0);    // não cabe em p, nem no ocupado
    DEPOIS();
    assert(q != p);
    assert((unsigned long)q + 50 == heap_top);  // foi para o fim
    dismiss_brk();
}

static void t_nenhum_cabe_cresce(void)
{
    setup_brk();
    void *a = memory_alloc(10, 0);
    void *b = memory_alloc(10, 0);
    memory_alloc(1, 0);
    memory_free(a);
    memory_free(b);
    unsigned long topo = heap_top;
    ANTES();
    void *c = memory_alloc(11, 1);
    DEPOIS();
    assert(c != a && c != b);
    assert((unsigned long)c == topo + 9);
    assert(heap_top == topo + 9 + 11);
    dismiss_brk();
}

// Dois vizinhos livres de 20 não viram um de 49
static void t_sem_fusao_de_vizinhos(void)
{
    setup_brk();
    void *a = memory_alloc(20, 0);
    void *b = memory_alloc(20, 0);
    memory_alloc(1, 0);
    memory_free(a);
    memory_free(b);
    unsigned long topo = heap_top;
    ANTES();
    void *c = memory_alloc(30, 0);
    DEPOIS();
    assert((unsigned long)c == topo + 9);
    dismiss_brk();
}

// estrategias

// cria livres de 30, 80, 50 separados por blocos ocupados
static void cria_livres(void **p30, void **p80, void **p50)
{
    *p30 = memory_alloc(30, 0); memory_alloc(1, 0);
    *p80 = memory_alloc(80, 0); memory_alloc(1, 0);
    *p50 = memory_alloc(50, 0); memory_alloc(1, 0);
    memory_free(*p30);
    memory_free(*p80);
    memory_free(*p50);
}

static void t_worst_fit_maior(void)
{
    void *p30, *p80, *p50;
    setup_brk();
    cria_livres(&p30, &p80, &p50);
    ANTES();
    assert(memory_alloc(20, 0) == p80);
    DEPOIS();
    dismiss_brk();
}

static void t_best_fit_menor(void)
{
    void *p30, *p80, *p50;
    setup_brk();
    cria_livres(&p30, &p80, &p50);
    ANTES();
    assert(memory_alloc(20, 1) == p30);
    DEPOIS();
    dismiss_brk();
}

static void t_best_fit_ignora_pequenos(void)
{
    void *p30, *p80, *p50;
    setup_brk();
    cria_livres(&p30, &p80, &p50);
    ANTES();
    assert(memory_alloc(40, 1) == p50);   // 30 não cabe
    DEPOIS();
    dismiss_brk();
}

static void t_estrategia_nao_zero_eh_best(void)
{
    void *p30, *p80, *p50;
    setup_brk();
    cria_livres(&p30, &p80, &p50);
    ANTES();
    assert(memory_alloc(20, 7) == p30);
    DEPOIS();
    dismiss_brk();
}

static void t_empate_best_pega_primeiro(void)
{
    setup_brk();
    void *a = memory_alloc(40, 0); memory_alloc(1, 0);
    void *b = memory_alloc(40, 0); memory_alloc(1, 0);
    memory_free(a);
    memory_free(b);
    ANTES();
    assert(memory_alloc(40, 1) == a);
    DEPOIS();
    dismiss_brk();
}

static void t_empate_worst_pega_primeiro(void)
{
    setup_brk();
    void *a = memory_alloc(40, 0); memory_alloc(1, 0);
    void *b = memory_alloc(40, 0); memory_alloc(1, 0);
    memory_free(a);
    memory_free(b);
    ANTES();
    assert(memory_alloc(40, 0) == a);
    DEPOIS();
    dismiss_brk();
}

// integridade dos dados

static void t_dados_preservados(void)
{
    setup_brk();
    unsigned char *a = memory_alloc(64, 0);
    unsigned char *b = memory_alloc(64, 0);
    unsigned char *c = memory_alloc(64, 0);
    memset(a, 0xAA, 64);
    memset(b, 0xBB, 64);
    memset(c, 0xCC, 64);

    memory_free(b);
    ANTES();
    unsigned char *d = memory_alloc(20, 1);   // divide b
    memset(d, 0xDD, 20);
    DEPOIS();

    for (int i = 0; i < 64; i++) {
        assert(a[i] == 0xAA);
        assert(c[i] == 0xCC);
    }
    assert(uso(a) == 1 && tamanho(a) == 64);
    assert(uso(c) == 1 && tamanho(c) == 64);
    dismiss_brk();
}

// dismiss e restart

static void t_dismiss_e_novo_setup(void)
{
    setup_brk();
    unsigned long inicio = heap_start;
    memory_alloc(1000, 0);
    ANTES();
    dismiss_brk();
    DEPOIS();
    assert(heap_top == inicio);

    setup_brk();
    assert(heap_start == inicio && heap_top == inicio);
    void *p = memory_alloc(10, 0);
    assert((unsigned long)p == inicio + 9);
    dismiss_brk();
}

// falhas

static void t_pedido_gigante_heap_vazia(void)
{
    setup_brk();
    ANTES();
    assert(memory_alloc((unsigned long)-1, 0) == NULL);
    assert(memory_alloc(1UL << 46, 0) == NULL);   //  mais ou menos 64 TiB
    DEPOIS();
    assert(heap_top == heap_start);               // nada mudou
    dismiss_brk();
}

static void t_pedido_gigante_com_livre(void)
{
    setup_brk();
    void *a = memory_alloc(100, 0);
    memory_alloc(1, 0);
    memory_free(a);
    unsigned long topo = heap_top;
    ANTES();
    assert(memory_alloc((unsigned long)-1, 1) == NULL);
    assert(memory_alloc((unsigned long)-1, 0) == NULL);
    DEPOIS();
    assert(uso(a) == 0 && tamanho(a) == 100);
    assert(heap_top == topo);
    dismiss_brk();
}

// testes de estresse (por favor funcionem)

#define N 2000

static void t_estresse(void)
{
    static void *v[N];
    setup_brk();
    for (int i = 0; i < N; i++) {
        v[i] = memory_alloc(i % 50 + 1, i % 2);
        assert(v[i] != NULL);
        memset(v[i], i & 0xFF, i % 50 + 1);
    }
    for (int i = 0; i < N; i += 2)
        memory_free(v[i]);

    ANTES();
    unsigned long topo = heap_top;
    for (int i = 0; i < N; i += 2) {
        void *p = memory_alloc(i % 50 + 1, 1);
        assert(p != NULL);
    }
    assert(heap_top == topo);     // tudo coube nos buracos
    DEPOIS();

    for (int i = 1; i < N; i += 2) {
        unsigned char *p = v[i];
        for (int k = 0; k < i % 50 + 1; k++)
            assert(p[k] == (i & 0xFF));
    }

    // percorre a heap e confere que os blocos fecham no topo
    unsigned long atual = heap_start;
    while (atual < heap_top) {
        unsigned char u = *(unsigned char *)atual;
        assert(u == 0 || u == 1);
        unsigned long t;
        memcpy(&t, (void *)(atual + 1), 8);
        atual += 9 + t;
    }
    assert(atual == heap_top);
    dismiss_brk();
}

// Executor

struct teste {
    const char *nome;
    void (*f)(void);
};

static struct teste testes[] = {
    {"setup: heap comeca vazia",                 t_setup_heap_vazia},
    {"alloc: primeira alocacao",                 t_primeira_alocacao},
    {"alloc: blocos contiguos no fim",           t_alocacoes_contiguas},
    {"alloc: tamanho zero",                      t_alocacao_tamanho_zero},
    {"free: marca livre, mantem tamanho",        t_free_marca_livre},
    {"free: nao mexe nos vizinhos",              t_free_nao_afeta_vizinhos},
    {"free: ponteiro fora da heap devolve 0",    t_free_fora_da_heap},
    {"free: primeiro, meio e ultimo bloco",      t_free_varios_blocos},
    {"free: bloco de tamanho 0 no fim",          t_free_bloco_zero_no_fim},
    {"reuso: tamanho exato",                     t_reuso_tamanho_exato},
    {"reuso: divide bloco grande",               t_divide_bloco},
    {"reuso: divide com sobra = 10",             t_divide_limite_10},
    {"reuso: nao divide com sobra = 9",          t_nao_divide_limite_9},
    {"reuso: sobra pode ser alocada",            t_sobra_reutilizavel},
    {"reuso: ignora bloco ocupado",              t_bloco_ocupado_ignorado},
    {"reuso: nenhum cabe, cresce a heap",        t_nenhum_cabe_cresce},
    {"reuso: sem fusao de livres vizinhos",      t_sem_fusao_de_vizinhos},
    {"worst fit: escolhe o maior",               t_worst_fit_maior},
    {"best fit: escolhe o menor",                t_best_fit_menor},
    {"best fit: ignora os que nao cabem",        t_best_fit_ignora_pequenos},
    {"estrategia ! =  0 funciona como best",       t_estrategia_nao_zero_eh_best},
    {"empate no best: pega o primeiro",          t_empate_best_pega_primeiro},
    {"empate no worst: pega o primeiro",         t_empate_worst_pega_primeiro},
    {"dados preservados entre operacoes",        t_dados_preservados},
    {"dismiss devolve brk e permite novo setup", t_dismiss_e_novo_setup},
    {"falha: pedido gigante, heap vazia",        t_pedido_gigante_heap_vazia},
    {"falha: pedido gigante com bloco livre",    t_pedido_gigante_com_livre},
    {"estresse: 2000 blocos",                    t_estresse},
};

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
        verbose = 1;

    // sem buffer no stdout: evita que o printf use malloc (que também mexe no brk) no meio dos testes
    setvbuf(stdout, NULL, _IONBF, 0);

    int total = sizeof testes / sizeof testes[0];
    int passou = 0;

    for (int i = 0; i < total; i++) {
        printf("[%2d] %-42s ", i + 1, testes[i].nome);
        if (verbose)
            printf("\n");
        pid_t pid = fork();
        if (pid == 0) {
            testes[i].f();
            _exit(0);
        }
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("OK\n");
            passou++;
        } else {
            printf("FALHOU\n");
        }
        if (verbose)
            printf("\n");
    }

    if (passou == total)
        return 0;
    return 1;
}
