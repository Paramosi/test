CC = cc
CPPFLAGS ?=
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wshadow
LDFLAGS ?=
LDLIBS ?= -lm

.PHONY: all clean
all: bno085_rpy

bno085_rpy: src/main.c src/bno085.c src/bno085.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c src/bno085.c $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -f bno085_rpy
