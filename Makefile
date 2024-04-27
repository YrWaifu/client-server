CC=gcc
CFLAGS=-Wall -Werror -Wextra

all: client server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

server: server.c
	$(CC) $(CFLAGS) server.c -o server

rebuild: clean all

clean:
	rm -rf server client