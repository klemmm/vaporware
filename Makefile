CC=gcc
EXTRA_WARNINGS=-Wall -Wextra -Wconversion -Wsign-compare -Wsign-conversion -W -Wshadow -Wunused-variable -Wunused-function -Wno-unused-parameter -Wunused -Wno-system-headers -Wwrite-strings 
CFLAGS=-O0 -g -fPIC $(EXTRA_WARNINGS) -DSTEAM_DLL=\"$(STEAM_DLL)\"
LDFLAGS=-shared


all: vaporware.so

vaporware.so: vaporware.o
	$(CC) $(LDFLAGS) -o $@ $^


vaporware.o: vaporware.c
ifndef STEAM_DLL
	$(error STEAM_DLL is not set)
endif

clean:
	rm -f *.so *.o *~

.PHONY: all clean

