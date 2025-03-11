CC=gcc
CFLAGS=
LDFLAGS=-lcrypto -lssl -lpthread -lasound

all: client server

client: ../src/client.c helper.o
	$(CC) $(CFLAGS) ../src/client.c helper.o -o client $(LDFLAGS)

server: ../src/server.c helper.o
	$(CC) $(CFLAGS) ../src/server.c helper.o -o server $(LDFLAGS)

helper.o: ../src/helper.c
	$(CC) $(CFLAGS) -c ../src/helper.c -o helper.o

test_client_send: ../src/test_client_send.c
	$(CC) $(CFLAGS) ../src/test_client_send.c -o test_client_send

test_client_read: ../src/test_client_read.c
	$(CC) $(CFLAGS) ../src/test_client_read.c -o test_client_read $(LDFLAGS)

rebuild: clean all

clean:
	rm -rf server client helper.o test_client_read test_client_send a.out
