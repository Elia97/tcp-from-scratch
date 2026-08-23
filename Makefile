CC = gcc

# Build normale
CFLAGS = -std=gnu11 -Wall -Wextra -O2
SANFLAGS = -fsanitize=address,undefined

# Assembly più leggibile per studio
ASMFLAGS = -std=gnu11 -Wall -Wextra -O0

.PHONY: all clean

all: server server.s

server: server.o
	$(CC) $(SANFLAGS) -o $@ $<

server.o: server.c
	$(CC) $(CFLAGS) $(SANFLAGS) -c -o $@ $<

server.s: server.c
	$(CC) $(ASMFLAGS) -S -o $@ $<

clean:
	rm -f server server.o server.s
