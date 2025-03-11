#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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

#define BUFFER_SIZE 4096
#define SOUND_BUFFER_SIZE 8000
#define MAX_CHANNEL_NAME_LENGTH 30
#define NICKNAME_SIZE 32

void *audio_streaming(void *arg);
void print_help();
void parse_arguments(int argc, char *argv[], int *port, char **ip);
int connect_to_server(char *ip, int port);
int process_command(char *buffer, int sock, char *ip, int port, int *connected_to_channel,
                    char channel_name[MAX_CHANNEL_NAME_LENGTH], char nickname[NICKNAME_SIZE], int *time_on,
                    int audio_sock);
void sha1_encode(const char *input_string, unsigned char *hash);
int sign_up(int sockfd);
int log_in(int sockfd);
void decode_and_display(char **encoded_lines, int num_messages);

int main(int argc, char *argv[]);

typedef struct {
    int client_socket;
    snd_pcm_t *capture_handle;
} record_data_t;

typedef struct {
    int client_socket;
    snd_pcm_t *playback_handle;
} playback_data_t;