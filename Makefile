CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: server client

server: src/server.c src/common.h
	$(CC) $(CFLAGS) -o server src/server.c

client: src/client.c src/common.h
	$(CC) $(CFLAGS) -o client src/client.c

clean:
	rm -f server client

.PHONY: all clean
