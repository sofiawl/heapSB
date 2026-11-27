#include <stdio.h>
#include "heap.h"

int main(void)
{
    setup_brk();
    void *a = memory_alloc(10, 0);
    void *z = memory_alloc(0, 0);
    int r = memory_free(z);
    printf("a = %p, z = %p, free devolveu %d (esperado 1)\n", a, z, r);
    dismiss_brk();
    return 0;
}
