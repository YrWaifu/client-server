CC = gcc
CFLAGS = 

CLIENT_SRC = client.c
SERVER_SRC = server.c

CLIENT_BIN = client
SERVER_BIN = server

CLIENT_LIBS = -lportaudio -lsndfile
SERVER_LIBS = -lsndfile -lportaudio -lSDL2

PORT = 6060

all: $(CLIENT_BIN) $(SERVER_BIN) client_listen

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIENT_LIBS)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(SERVER_LIBS)

client_listen: client_listen.c
	gcc client_listen.c -o client_listen -lportaudio -lpthread -lsndfile

killport:
	fuser -k $(PORT)/tcp

rebuild: clean all

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN) received_client.wav received.wav client_listen

.PHONY: all clean
