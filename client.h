#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define MAX_LOG_LINES 10
#define MAX_LOG_LINE_LENGTH 256

void print_help(char *argv[]);
void parse_arguments(int argc, char *argv[], int *port, char **ip);
int connect_to_server(char *ip, int port);
int process_command(char *buffer, int sock, char *ip, int port, int *connected_to_channel);

int main(int argc, char *argv[]);