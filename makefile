.PHONY: all clean

all: testes

# usamos --noexecstack para não ter aviso do ligador sobre pilha executável
heap.o: heap.s
	as -g --noexecstack heap.s -o heap.o

testes: main.c heap.h heap.o
	gcc -g -Wall -Wextra -no-pie main.c heap.o -o testes

clean:
	rm -f heap.o testes