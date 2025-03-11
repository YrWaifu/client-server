#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <getopt.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/helper.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048
#define MAX_CHANNELS 15
#define MAX_CHANNEL_NAME_LENGTH 30
#define MAX_COMMENT_LENGTH 1024
#define MAX_CHANNEL_CLIENTS 15
#define COMMAND_LEN 1024
#define SOUND_BUFFER_SIZE 4096

#define SO_REUSEPORT 15

#define NICKNAME_SIZE 32

struct Client;
struct Channel;

typedef struct Client {
    int socket;
    int sound_on;
    int sending_audio;
    int audio_socket;
    char audio_buffer[BUFFER_SIZE];
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

struct ServerContext {
    int server_running;
    Client *clients;
};

typedef struct ThreadData {
    struct Client *clients;  // Указатель на массив клиентов
    int server_paused;
    char sound_buffer[SOUND_BUFFER_SIZE];
    char last_buffer[BUFFER_SIZE];
} ThreadData;

#define MAX_AUDIO_CLIENTS 10  // Максимальное количество клиентов, передающих звук

typedef struct {
    int socket;
    int sound_on;
    int recording;
} AudioClient;

AudioClient audio_clients[MAX_AUDIO_CLIENTS];

void *audio_mixing_thread(void *arg);
void print_help();
void parse_arguments(int argc, char *argv[], int *port);
void setup_server(int *server_fd, int *audio_server_fd, int port, struct Client *clients,
                  struct sockaddr_in *address);
char *get_client_nickname(int socket, struct Client *clients);
int process_command(char *cmd, int server_fd, int audio_server_fd, struct Client *clients,
                    struct sockaddr_in *address, int *port, int *server_paused, Channel *channels,
                    int *num_channels);
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
void del_channel(Channel *channels, int *num_channels, char *channel_name, struct Client *clients);
void set_channel(Channel *channels, int num_channels, char *channel_name, char *new_comment);
void send_last_channel_messages(int sd, char *channel_name, int num_lines);
void send_channel_list(int client_socket, Channel *channels, int num_channels);
void print_server_info(int server_paused, time_t server_start_time, struct Client *clients, Channel *channels,
                       int num_channels);

void sha1_encode(const char *input_string, unsigned char *hash);

int main(int argc, char *argv[]);