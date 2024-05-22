#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void run_client(const char *ip, const char *port) {
    int pipe_fd[2];
    int input_pipe[2];
    if (pipe(pipe_fd) == -1 || pipe(input_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        close(pipe_fd[0]);                  // Close unused read end
        close(input_pipe[1]);               // Close unused write end
        dup2(pipe_fd[1], STDOUT_FILENO);    // Redirect stdout to pipe
        dup2(pipe_fd[1], STDERR_FILENO);    // Redirect stderr to pipe
        dup2(input_pipe[0], STDIN_FILENO);  // Redirect stdin to input pipe
        close(pipe_fd[1]);                  // Close original pipe write end
        close(input_pipe[0]);               // Close original pipe read end

        execl("./client", "./client", "-i", ip, "-p", port, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        close(pipe_fd[1]);     // Close unused write end
        close(input_pipe[0]);  // Close unused read end

        FILE *fp = fdopen(pipe_fd[0], "r");
        if (fp == NULL) {
            perror("fdopen");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));

        // Sending initial message
        sleep(1);
        write(input_pipe[1], "/join channel3\n", strlen("/join channel3\n"));
        fflush(NULL);  // Ensure all buffers are flushed

        for (int i = 0; i < 5; ++i) {
            sleep(1);
            write(input_pipe[1], "asd\n", strlen("asd\n"));
            fflush(NULL);  // Ensure all buffers are flushed

            // Read and print the output from the child process
            while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                printf("Output: %s", buffer);
                fflush(stdout);                     // Ensure stdout is flushed
                memset(buffer, 0, sizeof(buffer));  // Clear the buffer for next iteration
            }
        }

        // Sending exit message
        sleep(1);
        write(input_pipe[1], "/exit\n", strlen("/exit\n"));
        fflush(NULL);  // Ensure all buffers are flushed

        // Clean up
        fclose(fp);
        close(input_pipe[1]);
        wait(NULL);  // Wait for the child process to finish
    }
}

int main() {
    const char *ip = "127.0.0.1";
    const char *port = "8080";

    run_client(ip, port);

    return 0;
}
