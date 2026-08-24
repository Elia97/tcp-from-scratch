CC = gcc
BASE = -std=gnu11 -Wall -Wextra

SANFLAGS = -fsanitize=address,undefined

.PHONY: all debug release assembly clean

all: debug release

# Debug: osservabile, con i controlli runtime
DBGDIR = build/debug
DBGFLAGS = $(BASE) -g -O0 -fno-omit-frame-pointer

debug: $(DBGDIR)/server

$(DBGDIR)/server.o: server.c
	mkdir -p $(@D)
	$(CC) $(DBGFLAGS) $(SANFLAGS) -c -o $@ $<

$(DBGDIR)/server: $(DBGDIR)/server.o
	$(CC) $(SANFLAGS) -o $@ $<

# Release: ottimizzato, seconda opinione del compilatore
RELDIR = build/release
RELFLAGS = $(BASE) -O2

release: $(RELDIR)/server

$(RELDIR)/server.o: server.c
	mkdir -p $(@D)
	$(CC) $(RELFLAGS) -c -o $@ $<

$(RELDIR)/server: $(RELDIR)/server.o
	$(CC) -o $@ $<

# Assembly: ispezione, senza strumentazione
ASMDIR = build/assembly
ASMFLAGS = $(BASE) -O0

assembly: $(ASMDIR)/server.s

$(ASMDIR)/server.s: server.c
	mkdir -p $(@D)
	$(CC) $(ASMFLAGS) -S -o $@ $<

# Pulizia
clean:
	rm -rf build

