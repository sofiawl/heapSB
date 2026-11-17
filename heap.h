#ifndef HEAP_H
#define HEAP_H

void setup_brk(void);
void dismiss_brk(void);
void *memory_alloc(unsigned long int bytes, unsigned char strategy);
int memory_free(void *pointer);

#endif