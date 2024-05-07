CC=gcc
CFLAGS=
LDFLAGS=-lcrypto -lssl

all: client server

client: client.c helper.o
	$(CC) $(CFLAGS) client.c helper.o -o client $(LDFLAGS)

server: server.c helper.o
	$(CC) $(CFLAGS) server.c helper.o -o server $(LDFLAGS)

helper.o: helper.c
	$(CC) $(CFLAGS) -c helper.c -o helper.o

rebuild: clean all

clean:
	rm -rf server client helper.o
