#include "../include/server.h"

// Function to print the help message
void print_help() {
    printf("Usage: ./server -p {port}\n");
    printf("Options:\n");
    printf("  -p/--port {port}\tSpecifies the port number (default is 8080)\n");
    printf("  -h/--help\t\tDisplay this help message\n");
    printf("Commands:\n");
    printf("  /stop\t\tTo stop server\n");
    printf("  /pause\t\tTo pause server\n");
    printf("  /resume\tTo resume server\n");
    printf("  /change {PORT}\tTo change port\n");
    printf("  /status\tTo get status of the server\n");
    printf("  /add_channel {NAME} \"{COMMENT}\"\tTo add new channel\n");
    printf("  /set_channel {NAME} \"{COMMENT}\"\tChange existing channel\n");
    printf("  /del_channel {NAME}\tTo delete channel\n");
    printf("  /info {NAME}\tTo take info about channel\n");
    printf("  /channels\tList of channels\n");
}

// Function to parse command-line arguments
void parse_arguments(int argc, char *argv[], int *port) {
    int opt;

    // Define the long options for getopt
    struct option long_options[] = {
        {"port", required_argument, 0, 'p'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

    // Parse the arguments using getopt_long
    while ((opt = getopt_long(argc, argv, "p:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                *port = atoi(optarg);  // Set port from argument
                break;
            case 'h':
                print_help();  // Print help message
                break;
            default:
                fprintf(stderr, "Usage: %s -p {port}\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Set default port if not provided
    if (*port == 0) {
        *port = PORT;  // PORT should be defined elsewhere in your code
    }
}

// Function to set up the server
void setup_server(int *server_fd, int port, Client *clients, struct sockaddr_in *address) {
    // Create socket
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(port);

    // Bind the socket to the address and port
    if (bind(*server_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(*server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Initialize client sockets to 0 (not connected)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        strcpy(clients[i].nickname, "Unknown");
    }

    printf("Server listening on port %d\n", port);
}

// Function to get the nickname of a client by their socket
char *get_client_nickname(int socket, struct Client *clients) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == socket) {
            return clients[i].nickname;
        }
    }
    return "Unknown";
}

// Function to process commands from stdin
int process_command(char *cmd, int server_fd, struct Client *clients, struct sockaddr_in *address, int *port,
                    int *server_paused, Channel *channels, int *num_channels) {
    // Command "stop" or "exit"
    if (strcmp(cmd, "/stop") == 0 || strcmp(cmd, "/exit") == 0) {
        printf("Stopping the server...\n");
        close(server_fd);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].socket != 0) {
                close(clients[i].socket);
            }
        }
        exit(EXIT_SUCCESS);
        // Command "pause"
    } else if (strcmp(cmd, "/pause") == 0) {
        printf("Pausing the server...\n");
        *server_paused = 1;

        return 1;
        // Command "resume"
    } else if (strcmp(cmd, "/resume") == 0) {
        printf("Resuming the server...\n");
        *server_paused = 0;

        return 1;
        // Command "change" to change port
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
        // Command "status" to see number of connected clients
    } else if (strcmp(cmd, "/status") == 0) {
        int connected_clients = 0;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].socket != 0) {
                connected_clients++;
            }
        }
        printf("Number of connected clients: %d\n", connected_clients);
        return 1;
        // Command "info" to see channel info
    } else if (strncmp(cmd, "/info ", 6) == 0) {
        char channel_name[MAX_CHANNEL_NAME_LENGTH];
        strcpy(channel_name, cmd + 6);
        char *pos;

        if ((pos = strchr(channel_name, '\n')) != NULL) {
            *pos = '\0';
        }

        print_channel_info(channel_name, channels, *num_channels, clients);

        return 1;
        // Command "add_channel"
    } else if (strncmp(cmd, "/add_channel ", 13) == 0) {
        char *channel_start = cmd + 13;

        char *channel_end = strchr(channel_start, ' ');
        if (channel_end == NULL) {
            printf("Invalid command format: channel name is missing\n");
            return 1;
        }

        char *comment_start = strchr(channel_end, '\"');
        if (comment_start == NULL) {
            printf("Invalid command format: comment is missing\n");
            return 1;
        }

        char *comment_end = strchr(comment_start + 1, '\"');
        if (comment_end == NULL) {
            printf("Invalid command format: closing quote for comment is missing\n");
            return 1;
        }

        char channel_name[MAX_CHANNEL_NAME_LENGTH];
        int channel_name_length = channel_end - channel_start;
        if (channel_name_length >= MAX_CHANNEL_NAME_LENGTH) {
            printf("Channel name is too long\n");
            return 1;
        }
        strncpy(channel_name, channel_start, channel_name_length);
        channel_name[channel_name_length] = '\0';

        char comment[MAX_COMMENT_LENGTH];
        int comment_length = comment_end - comment_start - 1;
        if (comment_length >= MAX_COMMENT_LENGTH) {
            printf("Comment is too long\n");
            return 1;
        }
        strncpy(comment, comment_start + 1, comment_length);
        comment[comment_length] = '\0';

        add_channel(channels, channel_name, comment, num_channels);

        return 1;
        // Command "del_channel"
    } else if (strncmp(cmd, "/del_channel ", 13) == 0) {
        char channel_name[MAX_CHANNEL_NAME_LENGTH];
        strcpy(channel_name, cmd + 13);
        char *pos;

        if ((pos = strchr(channel_name, '\n')) != NULL) {
            *pos = '\0';
        }

        del_channel(channels, num_channels, channel_name);

        return 1;
        // Command "set_channel"
    } else if (strncmp(cmd, "/set_channel ", 13) == 0) {
        char *channel_start = cmd + 13;

        char *channel_end = strchr(channel_start, ' ');
        if (channel_end == NULL) {
            printf("Invalid command format: channel name is missing\n");
            return 1;
        }

        char *comment_start = strchr(channel_end, '\"');
        if (comment_start == NULL) {
            printf("Invalid command format: comment is missing\n");
            return 1;
        }

        char *comment_end = strchr(comment_start + 1, '\"');
        if (comment_end == NULL) {
            printf("Invalid command format: closing quote for comment is missing\n");
            return 1;
        }

        char channel_name[MAX_CHANNEL_NAME_LENGTH];
        int channel_name_length = channel_end - channel_start;
        if (channel_name_length >= MAX_CHANNEL_NAME_LENGTH) {
            printf("Channel name is too long\n");
            return 1;
        }
        strncpy(channel_name, channel_start, channel_name_length);
        channel_name[channel_name_length] = '\0';

        char new_comment[MAX_COMMENT_LENGTH];
        int comment_length = comment_end - comment_start - 1;
        if (comment_length >= MAX_COMMENT_LENGTH) {
            printf("Comment is too long\n");
            return 1;
        }
        strncpy(new_comment, comment_start + 1, comment_length);
        new_comment[comment_length] = '\0';

        set_channel(channels, *num_channels, channel_name, new_comment);

        return 1;
        // Command "help"
    } else if (strcmp(cmd, "/help") == 0) {
        print_help();
        return 1;
        // Command "channels"
    } else if ((strcmp(cmd, "/channels") == 0) || (strcmp(cmd, "/info") == 0)) {
        print_channels(channels, *num_channels);
        return 1;
    } else {
        printf("Wrong command\n");

        return 1;
    }
    return 0;
}

// Function to set a new comment for an existing channel
void set_channel(Channel *channels, int num_channels, char *channel_name, char *new_comment) {
    int found = 0;

    for (int i = 0; i < num_channels; i++) {
        if (strcmp(channels[i].channel, channel_name) == 0) {
            found = 1;
            strcpy(channels[i].comment, new_comment);
            break;
        }
    }

    if (found) {
        // Update the channels file with the new comment
        FILE *file = fopen("channels.txt", "w");
        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_channels; i++) {
            fprintf(file, "%s %s\n", channels[i].channel, channels[i].comment);
        }

        fclose(file);

        printf("Comment for channel '%s' successfully updated\n", channel_name);
    } else {
        printf("Channel '%s' not found\n", channel_name);
    }
}

// Function to delete a channel
void del_channel(Channel *channels, int *num_channels, char *channel_name) {
    int found = 0;
    int index = -1;

    for (int i = 0; i < *num_channels; i++) {
        if (strcmp(channels[i].channel, channel_name) == 0) {
            found = 1;
            index = i;
            break;
        }
    }

    if (found) {
        // Shift the channels array to remove the deleted channel
        for (int i = index; i < *num_channels - 1; i++) {
            strcpy(channels[i].channel, channels[i + 1].channel);
            strcpy(channels[i].comment, channels[i + 1].comment);
            channels[i].num_clients = channels[i + 1].num_clients;
        }

        (*num_channels)--;

        // Update the channels file
        FILE *file = fopen("channels.txt", "w");
        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < *num_channels; i++) {
            fprintf(file, "%s %s\n", channels[i].channel, channels[i].comment);
        }

        fclose(file);

        // Remove the channel log file
        char buffer[MAX_CHANNEL_NAME_LENGTH + 11];
        sprintf(buffer, "../logs/%s.log", channel_name);
        remove(buffer);

        printf("Channel '%s' successfully deleted\n", channel_name);
    } else {
        printf("Channel '%s' not found\n", channel_name);
    }
}

// Function to print information about a channel
void print_channel_info(char *channel_name, Channel *channels, int num_channels, struct Client *clients) {
    for (int i = 0; i < num_channels; ++i) {
        if (strcmp(channels[i].channel, channel_name) == 0) {
            printf("Channel: %s\n", channels[i].channel);
            printf("Comment: %s\n", channels[i].comment);
            printf("Connected clients:\n");

            // Print the list of clients connected to the channel
            for (int j = 0; j < channels[i].num_clients; ++j) {
                int client_socket = channels[i].clients[j].socket;
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                int client_port = ntohs(client_addr.sin_port);

                printf("  - %s (%s:%d)\n", channels[i].clients[j].nickname, client_ip, client_port);
            }

            return;
        }
    }
    printf("Channel '%s' not found\n", channel_name);
}

// Function to log a message
void log_message(char *channel_name, char *client_channel, char *client_nickname,
                 struct sockaddr_in client_address, char *buffer) {
    char log_filename[100];
    sprintf(log_filename, "../logs/%s.log", channel_name);
    FILE *log_file = fopen(log_filename, "a");

    if (log_file == NULL) {
        perror("Error opening log file");
    }

    // Get the current time
    char time_str[20];
    format_time(time_str);

    // Create a string with the message, time, date, and nickname
    char log_entry[1024];
    sprintf(log_entry, "[%s] %s (%s:%d): %s", time_str, client_nickname, inet_ntoa(client_address.sin_addr),
            ntohs(client_address.sin_port), buffer);

    // Generate the hash of the message including the time, date, and nickname
    char hash_input[1024];
    sprintf(hash_input, "%s%s%s", buffer, time_str, client_nickname);

    // printf("\n%s\n", hash_input);

    unsigned char hash[SHA_DIGEST_LENGTH];
    sha1_encode(hash_input, hash);

    // Write the log entry
    fprintf(log_file, "%s [Hash: ", log_entry);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        fprintf(log_file, "%02x", hash[i]);
    }
    fprintf(log_file, "]\n");

    fclose(log_file);
}

// Function to receive a message from a client
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
                // Decrease the number of clients in the channel the client was connected to
                for (int j = 0; j < num_channels; j++) {
                    if (strcmp(clients[i].channel, channels[j].channel) == 0) {
                        channels[j].num_clients--;
                        break;
                    }
                }
                clients[i].socket = 0;
                strcpy(clients[i].channel, "Unknown");
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

                // Update client channel
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket == sd) {
                        for (int j = 0; j < num_channels; j++) {
                            if (strcmp(clients[i].channel, channels[j].channel) == 0) {
                                channels[j].num_clients--;
                                break;
                            }
                        }

                        strcpy(clients[i].channel, channel_name);
                        printf("Client %s:%d connected to %s\n", inet_ntoa(client_address.sin_addr),
                               ntohs(client_address.sin_port), channel_name);
                        send(sd, "connected successfully\0", strlen("connected successfully\0"), 0);
                        add_client_to_channel(channels, num_channels, &clients[i], channel_name);
                        break;
                    }
                }
            } else if ((strncmp(buffer, "/read ", 6) == 0) || (strcmp(buffer, "/read\n") == 0)) {
                // Handle read command
                int num_lines = atoi(buffer + 6);
                if (num_lines <= 0) {
                    num_lines = MAX_LOG_LINES;
                }

                char *client_channel = get_client_channel(sd, clients);

                if (strcmp(client_channel, "Unknown") == 0) {
                    send(sd, "You are not connected to any channel\0",
                         strlen("You are not connected to any channel\0"), 0);
                } else {
                    send_last_channel_messages(sd, client_channel, num_lines);
                }

                return 1;
            } else if (strcmp(buffer, "/channels\n") == 0) {
                // Handle channels command
                send_channel_list(sd, channels, num_channels);

                return 1;
            } else if (strncmp(buffer, "NEWUSER ", 8) == 0) {
                // Handle new user registration
                char *user_data = buffer + 8;
                int user_exist = 0;
                char username[50], password_hash[SHA_DIGEST_LENGTH * 2 + 1];
                sscanf(user_data, "%[^:]:%s", username, password_hash);

                FILE *file = fopen("clients.txt", "r");
                if (file == NULL) {
                    perror("Could not open user file");
                    close(sd);
                    exit(1);
                }

                char file_line[MAX_LOG_LINE_LENGTH];
                int user_found = 0;
                while (fgets(file_line, sizeof(file_line), file)) {
                    char file_username[50], file_password_hash[SHA_DIGEST_LENGTH * 2 + 1];
                    sscanf(file_line, "%[^:]:%s %*[^]]", file_username, file_password_hash);

                    if (strcmp(username, file_username) == 0) {
                        user_exist = 1;
                    }
                }

                fclose(file);

                if (user_exist == 1) {
                    send(sd, "User exist\n", 15, 0);
                } else {
                    // Open the file again in append mode to add the new user
                    file = fopen("clients.txt", "a");
                    if (file == NULL) {
                        perror("Could not open user file for appending");
                        close(sd);
                        exit(1);
                    }

                    char date_str[20];
                    format_time(date_str);
                    fprintf(file, "%s [%s]\n", user_data, date_str);

                    fclose(file);
                    send(sd, "User created successfully\n", 27, 0);
                }

                return 1;
            } else if (strncmp(buffer, "USER ", 5) == 0) {
                // Handle user login
                char *user_data = buffer + 5;
                char username[50], password_hash[SHA_DIGEST_LENGTH * 2 + 1];
                sscanf(user_data, "%[^:]:%s", username, password_hash);

                FILE *file = fopen("clients.txt", "r");
                if (file == NULL) {
                    perror("Could not open user file");
                    close(sd);
                    exit(1);
                }

                char file_line[MAX_LOG_LINE_LENGTH];
                int user_found = 0;
                while (fgets(file_line, sizeof(file_line), file)) {
                    char file_username[50], file_password_hash[SHA_DIGEST_LENGTH * 2 + 1];
                    sscanf(file_line, "%[^:]:%s %*[^]]", file_username, file_password_hash);

                    if (strcmp(username, file_username) == 0) {
                        if (strcmp(password_hash, file_password_hash) == 0) {
                            send(sd, "Login successful\n", 18, 0);
                            user_found = 1;
                        } else {
                            send(sd, "Invalid password\n", 17, 0);
                            user_found = -1;
                        }
                        break;
                    }
                }

                if (user_found == 0) {
                    send(sd, "User not found\n", 15, 0);
                }

                fclose(file);

                return 1;
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

                // Get the current time
                char time_str[20];
                format_time(time_str);

                printf("[%s] %s -> %s (%s:%d): %s\n", time_str, client_channel, client_nickname,
                       inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), buffer);

                log_message(client_channel, client_channel, client_nickname, client_address, buffer);

                send(sd, "success\0", strlen("success\0"), 0);
            }
        } else {
            send(sd, "server is paused\0", strlen("server is paused\0"), 0);
        }
    }
    return 0;
}

// Function to send the list of channels to a client
void send_channel_list(int client_socket, Channel *channels, int num_channels) {
    char message[BUFFER_SIZE];
    char temp[BUFFER_SIZE];
    int message_length = 0;

    message_length += snprintf(message + message_length, BUFFER_SIZE - message_length,
                               "Total number of channels: %d\n", num_channels);
    message_length += snprintf(message + message_length, BUFFER_SIZE - message_length, "Channels:\n");
    for (int i = 0; i < num_channels; i++) {
        snprintf(temp, BUFFER_SIZE, "  %d. Name: %s, Comment: %s, Num of clients: %d\n", i + 1,
                 channels[i].channel, channels[i].comment, channels[i].num_clients);
        if (message_length + strlen(temp) < BUFFER_SIZE) {
            strcat(message, temp);
            message_length += strlen(temp);
        } else {
            send(client_socket, message, strlen(message), 0);
            strcpy(message, temp);
            message_length = strlen(temp);
        }
    }

    send(client_socket, message, strlen(message), 0);
}

// Function to format lines for output
char **format_lines(char **input_lines, int num_lines) {
    char **output_lines = (char **)malloc((num_lines + 3) * sizeof(char *));

    if (output_lines == NULL) {
        fprintf(stderr, "Memory error\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_lines + 3; i++) {
        output_lines[i] = (char *)malloc(MAX_OUTPUT_LINE_LENGTH * sizeof(char));
        if (output_lines[i] == NULL) {
            fprintf(stderr, "Memory error\n");
            exit(EXIT_FAILURE);
        }
        output_lines[i][0] = '\0';
    }

    sprintf(output_lines[0], "%s*%d*", READ_START, num_lines);

    int current_output_index = 1;
    int current_output_length = strlen(output_lines[0]);

    for (int i = 0; i < num_lines; i++) {
        char *newline_position = strchr(input_lines[i], '\n');
        if (newline_position != NULL) {
            *newline_position = '\0';
        }

        int input_length = strlen(input_lines[i]);

        if (current_output_length + input_length + strlen(DELIMITER_START) + strlen(DELIMITER_END) >=
            MAX_OUTPUT_LINE_LENGTH) {
            current_output_index++;
            current_output_length = 0;
        }

        if (current_output_length != 0) {
            strcat(output_lines[current_output_index], DELIMITER_START);
            current_output_length += strlen(DELIMITER_START);
        }

        strcat(output_lines[current_output_index], input_lines[i]);
        current_output_length += input_length;

        strcat(output_lines[current_output_index], DELIMITER_END);
        current_output_length += strlen(DELIMITER_END);
        // printf("\n%s\n", output_lines[current_output_index]);
    }

    // strcpy(output_lines[num_lines + 1], READ_END);

    // printf("\n%s\n%s\n%s\n", output_lines[0], output_lines[1], output_lines[3]);

    return output_lines;
}

// Function to send the last messages of a channel to a client
void send_last_channel_messages(int sd, char *channel_name, int number_log_lines) {
    char log_filename[100];
    sprintf(log_filename, "../logs/%s.log", channel_name);
    FILE *log_file = fopen(log_filename, "r");

    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    fseek(log_file, 0, SEEK_END);
    if (ftell(log_file) == 0) {
        char empty_channel_message[] = "Channel is currently empty.\n";
        send(sd, empty_channel_message, strlen(empty_channel_message), 0);
        fclose(log_file);
        return;
    }
    rewind(log_file);

    int num_lines = 0;
    int ch;
    while ((ch = fgetc(log_file)) != EOF) {
        if (ch == '\n') {
            num_lines++;
        }
    }

    rewind(log_file);

    int num_lines_to_skip = (num_lines > number_log_lines) ? num_lines - number_log_lines : 0;
    // printf("%d %d\n", num_lines_to_skip, num_lines);

    for (int i = 0; i < num_lines_to_skip; i++) {
        while ((ch = fgetc(log_file)) != '\n' && ch != EOF);
    }

    char *lines[number_log_lines];
    for (int i = 0; i < number_log_lines; i++) {
        lines[i] = (char *)malloc(MAX_LOG_LINE_LENGTH);
        // printf("%s", lines[i]);
        // fflush(stdout);
        memset(lines[i], 0, MAX_LOG_LINE_LENGTH);
    }

    int count = 0;

    char buffer[MAX_LOG_LINE_LENGTH];
    while (fgets(buffer, sizeof(buffer), log_file) != NULL) {
        strncpy(lines[count], buffer, MAX_LOG_LINE_LENGTH);
        count++;
    }

    // Format the lines
    char **formatted_lines = format_lines(lines, number_log_lines);
    // printf("%s %s", formatted_lines[0], formatted_lines[1]);

    // printf("\n%d\n", number_log_lines + 1);
    for (int i = 0; i < number_log_lines + 1; ++i) {
        send(sd, formatted_lines[i], MAX_LOG_LINE_LENGTH, 0);
    }

    fclose(log_file);
}

// Function to check if a channel exists
int channel_exists(Channel channels[], int num_channels, char *channel_name) {
    for (int i = 0; i < num_channels; i++) {
        if (strcmp(channels[i].channel, channel_name) == 0) {
            return 1;
        }
    }
    return 0;
}

// Function to handle new client connections
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

// Function to read channels from a file
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

// Function to add a new channel
void add_channel(Channel *channels, char *channel_name, char *comment, int *num_channels) {
    if (*num_channels >= MAX_CHANNELS) {
        printf("Maximum number of channels reached\n");
        return;
    }

    strcpy(channels[*num_channels].channel, channel_name);
    strcpy(channels[*num_channels].comment, comment);
    channels[*num_channels].num_clients = 0;

    (*num_channels)++;

    // Create log file for the new channel
    char log_filename[100];
    sprintf(log_filename, "../logs/%s.log", channel_name);
    FILE *log_file = fopen(log_filename, "w");

    if (log_file == NULL) {
        perror("Error creating log file");
        exit(EXIT_FAILURE);
    }

    fclose(log_file);

    // Append the new channel to the channels file
    FILE *file = fopen("channels.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s %s\n", channel_name, comment);
    fclose(file);

    printf("Channel '%s' added successfully with comment: '%s'\n", channel_name, comment);
}

// Function to print the list of channels
void print_channels(Channel *channels, int num_channels) {
    printf("Total number of channels: %d\n", num_channels);
    printf("Channels:\n");
    for (int i = 0; i < num_channels; i++) {
        printf("  %d. Name: %s, Comment: %s, Num of clients: %d\n", i + 1, channels[i].channel,
               channels[i].comment, channels[i].num_clients);
    }
}

// Function to add a client to a channel
void add_client_to_channel(Channel channels[], int num_channels, Client *client, char *channel_name) {
    for (int i = 0; i < num_channels; i++) {
        if (strcmp(channels[i].channel, channel_name) == 0) {
            if (channels[i].num_clients < MAX_CHANNEL_CLIENTS) {
                strcpy(channels[i].clients[channels[i].num_clients].nickname, client->nickname);
                channels[i].num_clients++;
                return;
            } else {
                return;
            }
        }
    }
}

// Function to get the channel a client is connected to
char *get_client_channel(int socket, struct Client *clients) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == socket) {
            return clients[i].channel;
        }
    }
    return "Unknown";
}

// Function to encode a string using SHA1
void sha1_encode(const char *input_string, unsigned char *hash) {
    SHA1((unsigned char *)input_string, strlen(input_string), hash);
}

int main(int argc, char *argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    fd_set readfds;
    int server_running = 1;
    int server_paused = 0;
    struct Client clients[MAX_CLIENTS] = {{0}};

    int num_channels = 0;
    Channel channels[MAX_CHANNELS];

    // Read channels from file
    read_channels_from_file(channels, &num_channels);
    print_channels(channels, num_channels);

    int port;

    // Parse command line arguments
    parse_arguments(argc, argv, &port);

    // Set up the server
    setup_server(&server_fd, port, clients, &address);

    while (server_running) {
        // Clean the fd set
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_sd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket != 0) {
                FD_SET(clients[i].socket, &readfds);
                if (clients[i].socket > max_sd) {
                    max_sd = clients[i].socket;
                }
            }
        }

        // Wait for activity on any of the sockets
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // New connection
        if (!server_paused && FD_ISSET(server_fd, &readfds)) {
            handle_new_connection(server_fd, &address, &addrlen, clients);
        }

        // Check for stdin input
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char cmd[COMMAND_LEN];

            if (fgets(cmd, sizeof(cmd), stdin) != NULL) {
                cmd[strcspn(cmd, "\n")] = 0;

                // Process command from stdin
                if (process_command(cmd, server_fd, clients, &address, &port, &server_paused, channels,
                                    &num_channels)) {
                    continue;
                }
            }
        }

        // Check for IO operation on existing client sockets
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
