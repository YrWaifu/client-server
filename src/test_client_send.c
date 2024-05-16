#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s -f <FILE_WITH_MESSAGES> -t <MESSAGE_PAUSE> [-r]\n", program_name);
}

int main(int argc, char *argv[]) {
    char *file_with_messages = NULL;
    int message_pause = 0;
    int repeat = 0;
    int opt;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "f:t:r")) != -1) {
        switch (opt) {
            case 'f':
                file_with_messages = optarg;
                break;
            case 't':
                message_pause = atoi(optarg);
                break;
            case 'r':
                repeat = 1;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (file_with_messages == NULL || message_pause <= 0) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_with_messages, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Read the first line for server IP, port, nick, and channel
    if ((read = getline(&line, &len, file)) == -1) {
        perror("Error reading file");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char server_ip[256], nick[256], channel[256];
    int port;
    sscanf(line, "%255[^:]:%d %255s %255s", server_ip, &port, nick, channel);

    // Construct the command
    char command[512];
    int snprintf_result = snprintf(command, sizeof(command), "./client -i %s -p %d", server_ip, port);
    if (snprintf_result < 0 || snprintf_result >= sizeof(command)) {
        fprintf(stderr, "Error: command string too long\n");
        free(line);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // int dev_null = open("/dev/null", O_WRONLY);
    // dup2(dev_null, STDOUT_FILENO);

    FILE *client = popen(command, "w");
    if (client == NULL) {
        perror("Error opening client process");
        free(line);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // close(dev_null);

    // Send nick and join commands
    fprintf(client, "/nick %s\n", nick);
    fflush(client);
    fprintf(client, "/join %s\n", channel);
    fflush(client);

    do {
        rewind(file);                // Go back to the beginning of the file
        getline(&line, &len, file);  // Skip the first line

        while ((read = getline(&line, &len, file)) != -1) {
            if (line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }

            // Write the message to the client process
            fprintf(client, "%s\n", line);
            fflush(client);

            // Sleep for the specified duration
            sleep(message_pause);
        }

    } while (repeat);

    fprintf(client, "/exit\n");
    fflush(client);

    // Close the pipe
    pclose(client);

    free(line);
    fclose(file);

    return 0;
}
