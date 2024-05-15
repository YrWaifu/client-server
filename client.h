#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "helper.h"

#define BUFFER_SIZE 2048
#define MAX_CHANNEL_NAME_LENGTH 30
#define NICKNAME_SIZE 32

void print_help(char *argv[]);
void parse_arguments(int argc, char *argv[], int *port, char **ip);
int connect_to_server(char *ip, int port);
int process_command(char *buffer, int sock, char *ip, int port, int *connected_to_channel,
                    char channel_name[MAX_CHANNEL_NAME_LENGTH], char nickname[NICKNAME_SIZE], int *time_on);

int main(int argc, char *argv[]);