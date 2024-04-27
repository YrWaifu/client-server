#include "client.h"

void print_help(char *argv[]) {
    printf("Usage: %s -p {PORT} -i {IP}\n", argv[0]);
    printf("Options:\n");
    printf("  -p/--port {PORT}\tSpecifies the port number\n");
    printf("  -i/--ip-address {IP}\tSpecifies the IP address\n");
    printf("  -h/--help\t\tDisplay this help message\n");
    printf("Commands:\n");
    printf("  /exit\t\tTo disconnect from server\n");
    printf("  /nick {NICK}\tTo enter your nickname\n");
    printf("  /join {CHANNEL} To join to channel\n");
    exit(EXIT_SUCCESS);
}

void parse_arguments(int argc, char *argv[], int *port, char **ip) {
    int opt;

    struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                    {"ip-address", required_argument, 0, 'i'},
                                    {"help", no_argument, 0, 'h'},
                                    {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "p:i:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                *port = atoi(optarg);
                break;
            case 'i':
                *ip = optarg;
                break;
            case 'h':
                print_help(argv);
                break;
            default:
                fprintf(stderr, "Usage: %s -p {PORT} -i {IP}\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (*port == 0 || *ip == NULL) {
        fprintf(stderr, "You need to enter port and ip\n");
        exit(EXIT_FAILURE);
    }
}

int connect_to_server(char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address / address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection Failed: Server is paused or unavailable\n");
        exit(EXIT_FAILURE);
    }

    return sock;
}

int process_command(char *buffer, int sock, char *ip, int port, int *connected_to_channel) {
    if (buffer[0] == '/') {
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("Disconnecting from server...\n");
            close(sock);
            exit(EXIT_SUCCESS);
        } else if (strncmp(buffer, "/nick", 5) == 0) {
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, BUFFER_SIZE);
            if (recv(sock, buffer, BUFFER_SIZE, 0) == -1) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            printf("Server: %s\n", buffer);

            return 1;
        } else if (strcmp(buffer, "/status\n") == 0) {
            printf("Current IP: %s, Port: %d\n", ip, port);

            return 1;
        } else if (strncmp(buffer, "/connect", 8) == 0) {
            char *new_ip = strtok(buffer + 8, " ");
            char *new_port = strtok(NULL, " ");

            if (new_ip == NULL || new_port == NULL) {
                printf("Usage: /connect <ip> <port>\n");
                return 1;
            }

            // close current connection
            close(sock);
            // reconnect to new server
            sock = connect_to_server(new_ip, atoi(new_port));

            return 1;
        } else if (strncmp(buffer, "/join", 5) == 0) {
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, BUFFER_SIZE);
            if (recv(sock, buffer, BUFFER_SIZE, 0) == -1) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            printf("Server: %s\n", buffer);
            *connected_to_channel = 1;

            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int sock = 0, valread;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};
    int connected_to_channel = 0;

    int port = 0;
    char *ip = NULL;

    parse_arguments(argc, argv, &port, &ip);

    sock = connect_to_server(ip, port);

    while (1) {
        printf("Enter message: ");
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, BUFFER_SIZE, stdin);

        // processing commands
        if (buffer[0] == '/') {
            if (process_command(buffer, sock, ip, port, &connected_to_channel)) {
                continue;
            }
        }

        // remove newline character from message
        if (!connected_to_channel) {
            printf("First connect to the channel /join {CHANNEL}\n");
            continue;
        }

        for (int i = 0; i < strlen(buffer); ++i) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';
            }
        }

        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));

        valread = read(sock, buffer, BUFFER_SIZE);
        printf("Server: %s\n", buffer);
    }
}