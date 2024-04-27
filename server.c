#include "server.h"

void print_help(char *argv[]) {
    printf("Usage: %s -p {port}\n", argv[0]);
    printf("Options:\n");
    printf("  -p/--port {port}\tSpecifies the port number (default is 8080)\n");
    printf("  -h/--help\t\tDisplay this help message\n");
    printf("Commands:\n");
    printf("  stop\t\tTo stop server\n");
    printf("  pause\t\tTo pause server\n");
    printf("  resume\tTo resume server\n");
    printf("  change {PORT}\tTo change port\n");
    printf("  status\tTo get status of the server\n");
    exit(EXIT_SUCCESS);
}

void parse_arguments(int argc, char *argv[], int *port) {
    int opt;

    struct option long_options[] = {
        {"port", required_argument, 0, 'p'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "p:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                *port = atoi(optarg);
                break;
            case 'h':
                print_help(argv);
                break;
            default:
                fprintf(stderr, "Usage: %s -p {port}\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (*port == 0) {
        *port = PORT;
    }
}

void setup_server(int *server_fd, int port, Client *clients, struct sockaddr_in *address) {
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(port);

    if (bind(*server_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(*server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        strcpy(clients[i].nickname, "Unknown");
    }

    printf("Server listening on port %d\n", port);
}

char *get_client_nickname(int socket, struct Client *clients) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == socket) {
            return clients[i].nickname;
        }
    }
    return "Unknown";
}

int process_command(char *cmd, int server_fd, struct Client *clients, struct sockaddr_in *address, int *port,
                    int *server_paused) {
    if (strcmp(cmd, "/stop") == 0 || strcmp(cmd, "/exit") == 0) {
        printf("Stopping the server...\n");
        close(server_fd);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].socket != 0) {
                close(clients[i].socket);
            }
        }
        exit(EXIT_SUCCESS);
    } else if (strcmp(cmd, "/pause") == 0) {
        printf("Pausing the server...\n");
        *server_paused = 1;

        return 1;
    } else if (strcmp(cmd, "/resume") == 0) {
        printf("Resuming the server...\n");
        *server_paused = 0;

        return 1;
    } else if (strncmp(cmd, "/change ", 8) == 0) {
        int new_port = atoi(cmd + 7);

        if (new_port > 0 && new_port < 65536) {
            printf("Changing server port to %d...\n", new_port);
            close(server_fd);
            *port = new_port;
            if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
                perror("socket failed");
                exit(EXIT_FAILURE);
            }
            if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int))) {
                perror("setsockopt");
                exit(EXIT_FAILURE);
            }
            address->sin_port = htons(*port);
            if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
                perror("bind failed");
                exit(EXIT_FAILURE);
            }
            printf("Server port changed to %d\n", *port);
            if (listen(server_fd, MAX_CLIENTS) < 0) {
                perror("listen");
                exit(EXIT_FAILURE);
            }

            return 1;
        } else {
            printf("Invalid port number\n");

            return 1;
        }
    } else if (strcmp(cmd, "/status") == 0) {
        int connected_clients = 0;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].socket != 0) {
                connected_clients++;
            }
        }
        printf("Number of connected clients: %d\n", connected_clients);
        return 1;
    } else {
        printf("Wrong command\n");
        return 1;
    }
    return 0;
}

int receive_message(int sd, char *buffer, struct Client *clients, int server_paused, Channel *channels,
                    int num_channels) {
    struct sockaddr_in client_address;
    int client_addrlen = sizeof(client_address);
    int valread;

    if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
        // Client disconnected
        getpeername(sd, (struct sockaddr *)&client_address, (socklen_t *)&client_addrlen);
        printf("Host disconnected, ip %s, port %d\n", inet_ntoa(client_address.sin_addr),
               ntohs(client_address.sin_port));
        close(sd);
        // Find and reset client socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == sd) {
                clients[i].socket = 0;
                break;
            }
        }
        return 1;
    } else {
        // Handle incoming message
        buffer[valread] = '\0';

        getpeername(sd, (struct sockaddr *)&client_address, (socklen_t *)&client_addrlen);
        if (!server_paused) {
            // Check if the message is a nickname change command
            if (strncmp(buffer, "/nick ", 6) == 0) {
                char new_nickname[NICKNAME_SIZE];
                strcpy(new_nickname, buffer + 6);
                char *pos;

                if ((pos = strchr(new_nickname, '\n')) != NULL) {
                    *pos = '\0';
                }

                // Set new nickname for the client
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket == sd) {
                        strcpy(clients[i].nickname, new_nickname);
                        printf("Client %s:%d changed nickname to %s\n", inet_ntoa(client_address.sin_addr),
                               ntohs(client_address.sin_port), new_nickname);
                        send(sd, "nickname changed successfully\0", strlen("nickname changed successfully\0"),
                             0);
                        break;
                    }
                }
            } else if (strncmp(buffer, "/join ", 6) == 0) {
                char channel_name[MAX_CHANNEL_NAME_LENGTH];
                strcpy(channel_name, buffer + 6);
                char *pos;

                if ((pos = strchr(channel_name, '\n')) != NULL) {
                    *pos = '\0';
                }

                if (!channel_exists(channels, num_channels, channel_name)) {
                    send(sd, "unknown channel\0", strlen("unknown channel\0"), 0);
                    return 1;
                }

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket == sd) {
                        strcpy(clients[i].channel, channel_name);
                        printf("Client %s:%d connected to %s\n", inet_ntoa(client_address.sin_addr),
                               ntohs(client_address.sin_port), channel_name);
                        send(sd, "connected successfully\0", strlen("connected successfully\0"), 0);
                        add_client_to_channel(channels, num_channels, &clients[i], channel_name);
                        break;
                    }
                }
            } else {
                // Echo received message back to client
                getpeername(sd, (struct sockaddr *)&client_address, (socklen_t *)&client_addrlen);
                char *client_nickname = get_client_nickname(sd, clients);
                char *client_channel = "Unknown";
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket == sd) {
                        client_channel = clients[i].channel;
                        break;
                    }
                }
                printf("%s -> %s (%s:%d): %s\n", client_channel, client_nickname,
                       inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), buffer);
                send(sd, "success\0", strlen("success\0"), 0);
            }
        } else {
            send(sd, "server is paused\0", strlen("server is paused\0"), 0);
        }
    }
    return 0;
}

int channel_exists(Channel channels[], int num_channels, char *channel_name) {
    for (int i = 0; i < num_channels; i++) {
        if (strcmp(channels[i].channel, channel_name) == 0) {
            return 1;
        }
    }
    return 0;
}

void handle_new_connection(int server_fd, struct sockaddr_in *address, int *addrlen, struct Client *clients) {
    int new_socket;
    if ((new_socket = accept(server_fd, (struct sockaddr *)address, (socklen_t *)addrlen)) == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Add new socket to array of client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = new_socket;
            clients[i].connected_to_channel = 0;
            strcpy(clients[i].channel, "Unknown");
            break;
        }
    }

    printf("New connection, ip %s, port %d\n", inet_ntoa(address->sin_addr), ntohs(address->sin_port));
}

void read_channels_from_file(Channel *channels, int *num_channels) {
    FILE *file = fopen("channels.txt", "r");

    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char line[MAX_CHANNEL_NAME_LENGTH + MAX_COMMENT_LENGTH + 2];
    char *space_pos;

    while (fgets(line, sizeof(line), file) != NULL && i < MAX_CHANNELS) {
        line[strcspn(line, "\n")] = '\0';
        space_pos = strchr(line, ' ');

        if (space_pos != NULL) {
            *space_pos = '\0';
            strcpy(channels[i].channel, line);
            strcpy(channels[i].comment, space_pos + 1);
            channels[i].num_clients = 0;
            (*num_channels)++;
            i++;
        }
    }

    fclose(file);
}

void add_channel(Channel *channels, char *channel_name, char *comment, int *num_channels) {
    if (*num_channels >= MAX_CHANNELS) {
        printf("Maximum number of channels reached\n");
        return;
    }

    strcpy(channels[*num_channels].channel, channel_name);
    strcpy(channels[*num_channels].comment, comment);
    channels[*num_channels].num_clients = 0;

    (*num_channels)++;

    FILE *file = fopen("channels.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s %s\n", channel_name, comment);
    fclose(file);

    printf("Channel '%s' added successfully with comment: '%s'\n", channel_name, comment);
}

void print_channels(Channel *channels, int num_channels) {
    printf("Total number of channels: %d\n", num_channels);
    printf("Channels:\n");
    for (int i = 0; i < num_channels; i++) {
        printf("  %d. Name: %s, Comment: %s, Num of clients: %d\n", i + 1, channels[i].channel,
               channels[i].comment, channels[i].num_clients);
    }
}

void add_client_to_channel(Channel channels[], int num_channels, Client *client, char *channel_name) {
    for (int i = 0; i < num_channels; i++) {
        if (strcmp(channels[i].channel, channel_name) == 0) {
            if (channels[i].num_clients < MAX_CHANNEL_CLIENTS) {
                strcpy(channels[i].clients[channels[i].num_clients].nickname, client->nickname);
                channels[i].num_clients++;
                printf("Client '%s' added to channel '%s'.\n", client->nickname, channel_name);
                return;
            } else {
                return;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    fd_set readfds;
    int server_running = 1;
    int server_paused = 0;
    struct Client clients[MAX_CLIENTS] = {{0}};

    int num_channels = 0;
    Channel channels[MAX_CHANNELS];

    read_channels_from_file(channels, &num_channels);
    print_channels(channels, num_channels);

    int port;

    parse_arguments(argc, argv, &port);

    setup_server(&server_fd, port, clients, &address);

    while (server_running) {
        // cleaning fd set
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_sd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != 0) {
                FD_SET(clients[i].socket, &readfds);
                if (clients[i].socket > max_sd) {
                    max_sd = clients[i].socket;
                }
            }
        }

        // waiting for activity on any of client sockets
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // new connection
        if (!server_paused && FD_ISSET(server_fd, &readfds)) {
            handle_new_connection(server_fd, &address, &addrlen, clients);
        }

        // stdin input
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char cmd[20];

            if (fgets(cmd, sizeof(cmd), stdin) != NULL) {
                cmd[strcspn(cmd, "\n")] = 0;

                if (process_command(cmd, server_fd, clients, &address, &port, &server_paused)) {
                    continue;
                }
            }
        }

        // check for IO operation on existing client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].socket;

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                // Call receive_message function
                if (receive_message(sd, buffer, clients, server_paused, channels, num_channels)) {
                    continue;
                }
            }
        }
    }

    return 0;
}