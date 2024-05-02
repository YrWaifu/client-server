#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define MAX_CHANNELS 15
#define MAX_CHANNEL_NAME_LENGTH 30
#define MAX_COMMENT_LENGTH 1024
#define MAX_CHANNEL_CLIENTS 15
#define COMMAND_LEN 1024

#define SO_REUSEPORT 15

#define NICKNAME_SIZE 32

#define MAX_LOG_LINES 10
#define MAX_LOG_LINE_LENGTH 256

struct Client;
struct Channel;

typedef struct Client {
    int socket;
    char nickname[NICKNAME_SIZE];
    char channel[MAX_CHANNEL_NAME_LENGTH];
    int connected_to_channel;
} Client;

typedef struct Channel {
    char channel[MAX_CHANNEL_NAME_LENGTH];
    char comment[MAX_COMMENT_LENGTH];
    Client clients[MAX_CHANNEL_CLIENTS];
    int num_clients;
} Channel;

void print_help(char *argv[]);
void parse_arguments(int argc, char *argv[], int *port);
void setup_server(int *server_fd, int port, struct Client *clients, struct sockaddr_in *address);
char *get_client_nickname(int socket, struct Client *clients);
int process_command(char *cmd, int server_fd, struct Client *clients, struct sockaddr_in *address, int *port,
                    int *server_paused, Channel *channels, int num_channels);
int receive_message(int sd, char *buffer, struct Client *clients, int server_paused, Channel *channels,
                    int num_channels);
void handle_new_connection(int server_fd, struct sockaddr_in *address, int *addrlen, struct Client *clients);

void print_channel_info(char *channel_name, Channel *channels, int num_channels, struct Client *clients);
void read_channels_from_file(Channel *channels, int *num_channels);
void add_channel(Channel *channels, char *channel_name, char *comment, int *num_channels);
void print_channels(Channel *channels, int num_channels);
int channel_exists(Channel channels[], int num_channels, char *channel_name);
char *get_client_channel(int socket, struct Client *clients);
void add_client_to_channel(Channel channels[], int num_channels, Client *client, char *channel_name);
void log_message(char *channel_name, char *client_channel, char *client_nickname,
                 struct sockaddr_in client_address, char *buffer);
void del_channel(Channel *channels, int *num_channels, char *channel_name);
void set_channel(Channel *channels, int num_channels, char *channel_name, char *new_comment);
void send_last_channel_messages(int sd, char *channel_name);

int main(int argc, char *argv[]);