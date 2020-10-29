CC=gcc
EXTRA_WARNINGS=-Wall -Wextra -Wconversion -Wsign-compare -Wsign-conversion -W -Wshadow -Wunused-variable -Wunused-function -Wno-unused-parameter -Wunused -Wno-system-headers -Wwrite-strings 
CFLAGS=-O0 -g -fPIC $(EXTRA_WARNINGS) 
LDFLAGS=-shared -ldl


all: vaporware.so

vaporware.so: vaporware.o
	$(CC) $(LDFLAGS) -o $@ $^


vaporware.o: vaporware.c vaporware.h

clean:
	rm -f *.so *.o *~

.PHONY: all clean

