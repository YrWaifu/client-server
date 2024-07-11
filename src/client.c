#include "../include/client.h"

// исправить на дефайн
int debug = 0;

// Function to print the help message
void print_help() {
    printf("Usage: ./client -p {PORT} -i {IP}\n");
    printf("Options:\n");
    printf("  -p/--port {PORT}\tSpecifies the port number\n");
    printf("  -i/--ip-address {IP}\tSpecifies the IP address\n");
    printf("  -h/--help\t\tDisplay this help message\n");
    printf("  -d/--debug\t\tDisplay hash in /read\n");
    printf("Commands:\n");
    printf("  /exit\t\tTo disconnect from server\n");
    printf("  /nick {NICK}\tTo enter your nickname\n");
    printf("  /join {CHANNEL} To join to channel\n");
    printf("  /read\tTo read last 10 messages from channel\n");
    printf("  /time\tTo switch on/off time\n");
    printf("  /channels\tList of channels\n");
}

// Function to encode a string using SHA1
void sha1_encode(const char *input_string, unsigned char *hash) {
    SHA1((unsigned char *)input_string, strlen(input_string), hash);
}

// Function to parse command line arguments
void parse_arguments(int argc, char *argv[], int *port, char **ip) {
    int opt;

    // Define the long options for getopt
    struct option long_options[] = {{"port", required_argument, 0, 'p'},
                                    {"ip-address", required_argument, 0, 'i'},
                                    {"help", no_argument, 0, 'h'},
                                    {"debug", no_argument, 0, 'd'},
                                    {0, 0, 0, 0}};

    // Parse the arguments using getopt_long
    while ((opt = getopt_long(argc, argv, "p:i:h:d", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                *port = atoi(optarg);  // Set the port number
                break;
            case 'i':
                *ip = optarg;  // Set the IP address
                break;
            case 'h':
                print_help();  // Print the help message
                break;
            case 'd':
                debug = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s -p {PORT} -i {IP}\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Check if port and IP are provided
    if (*port == 0 || *ip == NULL) {
        fprintf(stderr, "You need to enter port and ip\n");
        exit(EXIT_FAILURE);
    }
}

// Function to connect to the server
int connect_to_server(char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket
    if (sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);  // Set the port number

    // Convert IP address to binary form
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address / address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection Failed: Server is paused or unavailable\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    return sock;
}

// Function to process commands entered by the user
int process_command(char *buffer, int sock, char *ip, int port, int *connected_to_channel,
                    char channel_name[MAX_CHANNEL_NAME_LENGTH], char nickname[NICKNAME_SIZE], int *time_on) {
    if (buffer[0] == '/') {
        if (strcmp(buffer, "/exit\n") == 0) {
            // Handle the /exit command
            printf("Disconnecting from server...\n");
            fflush(stdout);
            close(sock);
            exit(EXIT_SUCCESS);
        } else if (strncmp(buffer, "/nick", 5) == 0) {
            // Handle the /nick command
            send(sock, buffer, strlen(buffer), 0);

            char name[NICKNAME_SIZE];
            strcpy(name, buffer + 6);  // Extract the nickname
            char *pos;
            if ((pos = strchr(name, '\n')) != NULL) {
                *pos = '\0';  // Remove the newline character
            }
            strncpy(nickname, name, NICKNAME_SIZE);  // Set the nickname

            memset(buffer, 0, BUFFER_SIZE);

            if (recv(sock, buffer, BUFFER_SIZE, 0) == -1) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            if (*time_on) {
                // Display time if time_on is enabled
                char time_str[20];
                format_time(time_str);

                printf("[%s] Server: %s\n", time_str, buffer);
                fflush(stdout);
            } else {
                printf("Server: %s\n", buffer);
                fflush(stdout);
            }

            return 1;
        } else if (strcmp(buffer, "/status\n") == 0) {
            // Handle the /status command
            printf("Current IP: %s, Port: %d\n", ip, port);
            fflush(stdout);

            return 1;
        } else if (strncmp(buffer, "/connect", 8) == 0) {
            // Handle the /connect command
            char *new_ip = strtok(buffer + 8, " ");
            char *new_port = strtok(NULL, " ");

            if (new_ip == NULL || new_port == NULL) {
                printf("Usage: /connect <ip> <port>\n");
                fflush(stdout);
                return 1;
            }

            // Close current connection and reconnect to new server
            close(sock);
            sock = connect_to_server(new_ip, atoi(new_port));

            return 1;
        } else if (strncmp(buffer, "/join", 5) == 0) {
            // Handle the /join command
            send(sock, buffer, strlen(buffer), 0);

            char name[MAX_CHANNEL_NAME_LENGTH];
            strcpy(name, buffer + 6);  // Extract the channel name
            char *pos;
            if ((pos = strchr(name, '\n')) != NULL) {
                *pos = '\0';  // Remove the newline character
            }
            strncpy(channel_name, name, MAX_CHANNEL_NAME_LENGTH);  // Set the channel name

            memset(buffer, 0, BUFFER_SIZE);
            if (recv(sock, buffer, BUFFER_SIZE, 0) == -1) {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            if (*time_on) {
                // Display time if time_on is enabled
                char time_str[20];
                format_time(time_str);

                printf("[%s] Server: %s\n", time_str, buffer);
                fflush(stdout);
            } else {
                printf("Server: %s\n", buffer);
                fflush(stdout);
            }

            // Check if the channel is unknown
            if (strcmp(buffer, "unknown channel") == 0) {
                *connected_to_channel = 0;
                strncpy(channel_name, "Unknown", MAX_CHANNEL_NAME_LENGTH);
                channel_name[MAX_CHANNEL_NAME_LENGTH - 1] = '\0';
            } else {
                *connected_to_channel = 1;
            }

            return 1;
        } else if ((strncmp(buffer, "/read ", 6) == 0) || (strcmp(buffer, "/read\n") == 0)) {
            // Handle the /read command
            int num_lines = atoi(buffer + 6);
            if (num_lines <= 0) {
                num_lines = MAX_LOG_LINES;
            }

            send(sock, buffer, strlen(buffer), 0);

            char **output_lines = (char **)malloc((num_lines + 1) * sizeof(char *));

            if (output_lines == NULL) {
                fprintf(stderr, "Memory error\n");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < num_lines + 1; i++) {
                output_lines[i] = (char *)malloc(MAX_OUTPUT_LINE_LENGTH * sizeof(char));
                if (output_lines[i] == NULL) {
                    fprintf(stderr, "Memory error\n");
                    exit(EXIT_FAILURE);
                }
                output_lines[i][0] = '\0';
            }

            for (int i = 0; i < num_lines + 1; ++i) {
                recv(sock, output_lines[i], MAX_LOG_LINE_LENGTH, 0);
                if (strcmp(output_lines[i], "You are not connected to any channel") == 0) {
                    printf("%s\n", output_lines[i]);
                    return 1;
                }
                if (strcmp(output_lines[i], "Channel is currently empty.\n") == 0) {
                    printf("%s", output_lines[i]);
                    return 1;
                }
                // printf("------\n%s\n", output_lines[i]);
                fflush(stdout);
            }

            decode_and_display(output_lines, num_lines + 1);

            fflush(stdout);

            return 1;
        } else if (strcmp(buffer, "/channels\n") == 0) {
            // Handle the /channels command
            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                perror("send");
                return -1;
            }

            char response[BUFFER_SIZE];
            memset(response, 0, sizeof(response));
            if (recv(sock, response, sizeof(response), 0) < 0) {
                perror("recv");
                return -1;
            }

            printf("%s", response);
            fflush(stdout);

            return 1;
        } else if (strcmp(buffer, "/help\n") == 0) {
            // Handle the /help command
            print_help();

            return 1;
        } else if (strcmp(buffer, "/time\n") == 0) {
            // Toggle the time display
            *time_on = *time_on == 1 ? 0 : 1;

            return 1;
        }
    }
    return 0;
}

// Function to decode and display messages
void decode_and_display(char **encoded_lines, int num_messages) {
    for (int i = 0; i < num_messages; ++i) {
        char *line = encoded_lines[i];
        char *start = strstr(line, DELIMITER_START);
        while (start != NULL) {
            start += strlen(DELIMITER_START);
            char *end = strstr(start, DELIMITER_END);
            if (end != NULL) {
                *end = '\0';
                if (strcmp(start, "") != 0) {
                    if (debug == 0) {
                        // Deleting hash from line
                        char *hash_pos = strstr(start, " [Hash: ");
                        if (hash_pos != NULL) {
                            *hash_pos = '\0';
                        }
                    }
                    printf("%s\n", start);
                    fflush(stdout);
                }
                start = end + strlen(DELIMITER_END);
            } else {
                break;
            }
        }
    }
}

// Function to sign up a new user
int sign_up(int sockfd) {
    char username[50];
    char password[50];
    char send_str[1024];
    char hash_str[SHA_DIGEST_LENGTH * 2 + 1];
    char recv_str[1024];
    int recv_len;

    printf("Register a new user\n");
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);
    fflush(stdout);

    // Encode the password using SHA1
    unsigned char hash[SHA_DIGEST_LENGTH];
    sha1_encode(password, hash);

    // Convert the hash to a hexadecimal string
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&hash_str[i * 2], "%02x", hash[i]);
    }

    // Prepare the sign-up message
    sprintf(send_str, "NEWUSER %s:%s", username, hash_str);

    // Send the sign-up message to the server
    send(sockfd, send_str, strlen(send_str), 0);

    // Receive the response from the server
    recv_len = recv(sockfd, recv_str, 1024, 0);
    recv_str[recv_len] = '\0';
    printf("Server: %s", recv_str);
    fflush(stdout);

    // Check if the user was created successfully
    if (strcmp(recv_str, "User created successfully\n") == 0) {
        return 1;
    }

    return 0;
}

// Function to log in an existing user
int log_in(int sockfd) {
    char username[50];
    char password[50];
    unsigned char hash[SHA_DIGEST_LENGTH];
    char hash_str[2 * SHA_DIGEST_LENGTH + 1];
    char send_str[1024];
    char recv_str[1024];
    int recv_len;

    printf("Login to an existing account\n");
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);
    fflush(stdout);

    // Encode the password using SHA1
    sha1_encode(password, hash);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&hash_str[i * 2], "%02x", hash[i]);
    }

    // Prepare the login message
    sprintf(send_str, "USER %s:%s", username, hash_str);
    send(sockfd, send_str, strlen(send_str), 0);

    // Receive the response from the server
    recv_len = recv(sockfd, recv_str, 1024, 0);
    recv_str[recv_len] = '\0';
    printf("Server: %s", recv_str);
    fflush(stdout);

    // Check if the login was successful
    if (strcmp(recv_str, "Login successful\n") == 0) {
        return 1;
    }

    return 0;
}

// Main function
int main(int argc, char *argv[]) {
    int sock = 0, valread;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};
    int connected_to_channel = 0;
    char channel_name[MAX_CHANNEL_NAME_LENGTH] = "Unknown";
    char nickname[NICKNAME_SIZE] = "Unknown";
    int time_on = 0;

    int port = 0;
    char *ip = NULL;

    // Parse command line arguments
    parse_arguments(argc, argv, &port, &ip);

    // Connect to the server
    sock = connect_to_server(ip, port);

    int choice = 0;
    int loggedIn = 0;
    while (loggedIn == 0) {
        // Prompt the user to sign up or log in
        printf("Press the button:\n1 - Sign up\n2 - Log in\n");
        fflush(stdout);
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                loggedIn = sign_up(sock);  // Sign up a new user
                break;
            case 2:
                loggedIn = log_in(sock);  // Log in an existing user
                break;
            default:
                printf("Wrong choice\n");
        }
    }

    while (1) {
        // Display the prompt
        printf("%s@%s::", nickname, channel_name);
        fflush(stdin);
        fflush(stdout);
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, BUFFER_SIZE, stdin);

        // Process commands
        if (buffer[0] == '/') {
            if (process_command(buffer, sock, ip, port, &connected_to_channel, channel_name, nickname,
                                &time_on)) {
                continue;
            }
        }

        // Remove newline character from message
        if (!connected_to_channel) {
            printf("First connect to the channel /join {CHANNEL}\n");
            fflush(stdout);
            continue;
        }

        for (int i = 0; i < strlen(buffer); ++i) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';
            }
        }

        // Send the message to the server
        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));

        // Read the server's response
        valread = read(sock, buffer, BUFFER_SIZE);

        if (time_on) {
            // Display time if time_on is enabled
            char time_str[20];
            format_time(time_str);

            printf("[%s] Server: %s\n", time_str, buffer);
            fflush(stdout);
        } else {
            printf("Server: %s\n", buffer);
            fflush(stdout);
        }
    }
}
