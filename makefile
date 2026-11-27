.PHONY: all clean

all: programa

heap.o: heap.s
	as -g heap.s -o heap.o

programa: main.c heap.h heap.o
	gcc -g -Wall -Wextra -no-pie main.c heap.o -o prog

clean:
	rm -f heap.o programa
