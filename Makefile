CC=gcc
CFLAGS=
LDFLAGS=-lcrypto -lssl

all: client server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

server: server.c
	$(CC) $(CFLAGS) server.c -o server $(LDFLAGS)

rebuild: clean all

clean:
	rm -rf server client