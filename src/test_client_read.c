#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <openssl/sha.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s -f <FILE_WITH_MESSAGES>\n", program_name);
}

void sha1_encode(const char *input_string, unsigned char *hash) {
    SHA1((unsigned char *)input_string, strlen(input_string), hash);
}

int main(int argc, char *argv[]) {
    char *file_with_messages = NULL;
    int opt;
    int test_started = 0;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
            case 'f':
                file_with_messages = optarg;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (file_with_messages == NULL) {
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

    // Debug: Print the command to be executed
    printf("Executing command: %s\n", command);

    // Open the client process for reading and writing
    FILE *client = popen(command, "w");
    if (client == NULL) {
        perror("Error opening client process");
        free(line);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Send nick and join commands
    fprintf(client, "/nick Checker\n");
    fflush(client);
    fprintf(client, "/join %s\n", channel);
    fflush(client);

    // Read messages and compute their hashes
    char *messages[256];
    unsigned char hashes[256][SHA_DIGEST_LENGTH];
    int message_count = 0;

    while ((read = getline(&line, &len, file)) != -1) {
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        messages[message_count] = strdup(line);
        sha1_encode(line, hashes[message_count]);
        message_count++;
    }

    free(line);
    fclose(file);

    int message_index = 0;
    int success = 1;  // Flag to indicate if all hashes match
    char client_output[1024];

    while (message_index < message_count) {
        // Send /read command to the client process
        fprintf(client, "/read\n");
        fflush(client);

        // Read the client output
        while (fgets(client_output, sizeof(client_output), client) != NULL) {
            // Print the output for debugging purposes
            printf("%s", client_output);

            // Check if the test has started
            if (!test_started && strstr(client_output, "TEST STARTED")) {
                test_started = 1;
                printf("Test started!\n");
            }

            // If test has started, compare hashes
            if (test_started) {
                if (strstr(client_output, messages[message_index])) {
                    unsigned char hash[SHA_DIGEST_LENGTH];
                    sha1_encode(messages[message_index], hash);
                    if (memcmp(hash, hashes[message_index], SHA_DIGEST_LENGTH) == 0) {
                        printf("Message '%s' hash matches\n", messages[message_index]);
                    } else {
                        printf("Message '%s' hash does not match\n", messages[message_index]);
                        success = 0;
                    }
                    message_index++;
                    break;
                }
            }
        }

        // Sleep for the specified duration
        sleep(1);
    }

    fprintf(client, "/exit\n");
    fflush(client);

    // Close the pipe
    pclose(client);

    // Free allocated memory for messages
    for (int i = 0; i < message_count; i++) {
        free(messages[i]);
    }

    // Print the final result
    if (success) {
        printf("SUCCESS\n");
    } else {
        printf("FAIL\n");
    }

    return 0;
}
