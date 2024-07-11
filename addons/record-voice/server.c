#include <arpa/inet.h>
#include <portaudio.h>
#include <pthread.h>
#include <signal.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define PORT 6060
#define FILE_NAME "received.wav"
#define CLIENT_FILE_NAME "received_client.wav"
#define BUFFER_SIZE 11025  // Синхронизируем размер буфера с клиентом
#define MAX_CLIENTS 10

typedef struct {
    float buffer[BUFFER_SIZE * NUM_CHANNELS];
    size_t buffer_size;
} PaData;

typedef struct {
    int client_socks[MAX_CLIENTS];
    int client_count;
    int primary_client_sock;
    volatile sig_atomic_t running;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ServerData;

ServerData serverData;
pthread_t command_thread;

void handle_sigpipe(int sig) { printf("Client disconnected.\n"); }

void add_client(int client_sock) {
    pthread_mutex_lock(&serverData.mutex);
    if (serverData.client_count < MAX_CLIENTS) {
        serverData.client_socks[serverData.client_count++] = client_sock;
    } else {
        printf("Max clients reached. Unable to add new client.\n");
    }
    pthread_mutex_unlock(&serverData.mutex);
}

void remove_client(int client_sock) {
    pthread_mutex_lock(&serverData.mutex);
    for (int i = 0; i < serverData.client_count; i++) {
        if (serverData.client_socks[i] == client_sock) {
            serverData.client_socks[i] = serverData.client_socks[--serverData.client_count];
            break;
        }
    }
    pthread_mutex_unlock(&serverData.mutex);
}

void broadcast_to_clients(const float *buffer, size_t buffer_size) {
    pthread_mutex_lock(&serverData.mutex);
    for (int i = 0; i < serverData.client_count; i++) {
        if (serverData.client_socks[i] != serverData.primary_client_sock) {
            send(serverData.client_socks[i], buffer, buffer_size * sizeof(float), 0);
        }
    }
    pthread_mutex_unlock(&serverData.mutex);
}

static int playCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                        void *userData) {
    PaData *data = (PaData *)userData;
    float *out = (float *)outputBuffer;
    (void)inputBuffer;

    if (data->buffer_size > 0) {
        memcpy(out, data->buffer, data->buffer_size * sizeof(float));
        data->buffer_size = 0;
    } else {
        memset(out, 0, framesPerBuffer * NUM_CHANNELS * sizeof(float));
    }

    return paContinue;
}

void *commandHandler(void *arg) {
    char command[16];
    while (1) {
        printf("Enter command (exit): ");
        if (scanf("%15s", command) == 1) {
            pthread_mutex_lock(&serverData.mutex);

            if (strcmp(command, "exit") == 0) {
                serverData.running = 0;
                pthread_cond_signal(&serverData.cond);
                pthread_mutex_unlock(&serverData.mutex);
                exit(0);
            }

            pthread_mutex_unlock(&serverData.mutex);
        }
    }
    return NULL;
}

void process_client(int client_sock) {
    SF_INFO sfinfo;
    ssize_t n;
    PaError err;
    PaStream *stream;
    PaData pa_data = {0};
    SNDFILE *outfile = NULL, *client_outfile = NULL;

    if (serverData.primary_client_sock == -1) {
        serverData.primary_client_sock = client_sock;
        memset(&sfinfo, 0, sizeof(sfinfo));
        sfinfo.samplerate = SAMPLE_RATE;
        sfinfo.channels = NUM_CHANNELS;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

        outfile = sf_open(FILE_NAME, SFM_WRITE, &sfinfo);
        if (outfile == NULL) {
            printf("Could not open file for writing: %s\n", sf_strerror(NULL));
            close(client_sock);
            return;
        }

        client_outfile = sf_open(CLIENT_FILE_NAME, SFM_WRITE, &sfinfo);
        if (client_outfile == NULL) {
            printf("Could not open client file for writing: %s\n", sf_strerror(NULL));
            sf_close(outfile);
            close(client_sock);
            return;
        }

        err = Pa_Initialize();
        if (err != paNoError) {
            printf("PortAudio error: %s\n", Pa_GetErrorText(err));
            sf_close(outfile);
            sf_close(client_outfile);
            close(client_sock);
            return;
        }

        err = Pa_OpenDefaultStream(&stream, 0, NUM_CHANNELS, paFloat32, SAMPLE_RATE, BUFFER_SIZE,
                                   playCallback, &pa_data);
        if (err != paNoError) {
            printf("PortAudio error: %s\n", Pa_GetErrorText(err));
            Pa_Terminate();
            sf_close(outfile);
            sf_close(client_outfile);
            close(client_sock);
            return;
        }

        err = Pa_StartStream(stream);
        if (err != paNoError) {
            printf("PortAudio error: %s\n", Pa_GetErrorText(err));
            Pa_CloseStream(stream);
            Pa_Terminate();
            sf_close(outfile);
            sf_close(client_outfile);
            close(client_sock);
            return;
        }
    }

    while ((n = recv(client_sock, pa_data.buffer, sizeof(pa_data.buffer), 0)) > 0) {
        pa_data.buffer_size = n / sizeof(float);
        if (outfile) {
            sf_write_float(outfile, pa_data.buffer, pa_data.buffer_size);
            broadcast_to_clients(pa_data.buffer, pa_data.buffer_size);
            memcpy(pa_data.buffer, pa_data.buffer,
                   pa_data.buffer_size * sizeof(float));  // Дублируем данные для воспроизведения
        }
        if (client_outfile) {
            sf_write_float(client_outfile, pa_data.buffer, pa_data.buffer_size);
        }
    }

    if (n == 0) {
        printf("Client closed the connection.\n");
    } else if (n < 0) {
        perror("Error receiving data");
    }

    if (outfile) {
        sf_close(outfile);
    }
    if (client_outfile) {
        sf_close(client_outfile);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    close(client_sock);
    remove_client(client_sock);

    if (client_sock == serverData.primary_client_sock) {
        serverData.primary_client_sock = -1;
    }
    printf("Recording received and saved to file.\n");
}

int main() {
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    serverData.running = 1;
    serverData.client_count = 0;
    serverData.primary_client_sock = -1;
    pthread_mutex_init(&serverData.mutex, NULL);
    pthread_cond_init(&serverData.cond, NULL);

    signal(SIGPIPE, handle_sigpipe);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        close(sockfd);
        exit(1);
    }

    if (pthread_create(&command_thread, NULL, commandHandler, NULL) != 0) {
        perror("Error creating command handler thread");
        return 1;
    }

    listen(sockfd, 5);

    while (serverData.running) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            if (serverData.running) {
                perror("Error on accept");
            }
            continue;
        }
        add_client(newsockfd);
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, (void *(*)(void *))process_client,
                           (void *)(intptr_t)newsockfd) != 0) {
            perror("Error creating client handler thread");
            remove_client(newsockfd);
            close(newsockfd);
        }
        pthread_detach(client_thread);
    }

    close(sockfd);
    pthread_mutex_destroy(&serverData.mutex);
    pthread_cond_destroy(&serverData.cond);
    return 0;
}
