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

test_client_send: test_client_send.c
	$(CC) $(CFLAGS) -c test_client_send.c -o test_client_send

test_client_read: test_client_read.c
	$(CC) $(CFLAGS) -c test_client_read.c -o test_client_read

rebuild: clean all

clean:
	rm -rf server client helper.o
