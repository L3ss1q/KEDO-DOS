CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -I../kernel/include
SOURCES = ../kernel/src/main.c ../kernel/src/shell.c ../kernel/src/fs.c ../kernel/src/error.c

all: kedo

kedo: $(SOURCES) ../kernel/include/kedo.h ../kernel/include/kedoError.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SOURCES) -o $@

clean:
	rm -f kedo kedo.exe

.PHONY: all clean
