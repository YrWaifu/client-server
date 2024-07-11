#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 8000

void record_audio(char* buffer, snd_pcm_t* capture_handle) {
    snd_pcm_readi(capture_handle, buffer, BUFFER_SIZE / 2);
}

void play_audio(const char* buffer, snd_pcm_t* playback_handle) {
    snd_pcm_writei(playback_handle, buffer, BUFFER_SIZE / 2);
}

int main(int argc, char* argv[]) {
    int client_socket;
    struct sockaddr_in serv_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        return -1;
    }

    if (connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    snd_pcm_t *capture_handle = nullptr, *playback_handle;  // дескрипторы устройств
    std::ifstream audio_file;
    bool use_file = false;

    if (argc == 2) {
        audio_file.open(argv[1], std::ios::binary);
        if (!audio_file.is_open()) {
            std::cerr << "Failed to open audio file: " << argv[1] << std::endl;
            return -1;
        }
        use_file = true;
    } else {
        // дескриптор, звуковое устройство, которое нужно открыть, тип потока (в данном случае захват),
        // настройки по умолчанию
        snd_pcm_open(&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
        // дескриптор, формат звука (16ти битный), способ доступа к данным (каналы чередуются), одноканал,
        // частота дискретизации, ресемплинг, задержка в микосекундах.
        snd_pcm_set_params(capture_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, 44100, 1,
                           100000);
    }

    snd_pcm_open(&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_set_params(playback_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, 44100, 1,
                       100000);

    char buffer[BUFFER_SIZE];

    while (true) {
        if (use_file) {
            audio_file.read(buffer, BUFFER_SIZE);
            if (audio_file.eof()) {
                break;
            }
        } else {
            record_audio(buffer, capture_handle);
        }

        send(client_socket, buffer, BUFFER_SIZE, 0);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }

        if (use_file) {
            // std::cout << "Playing silence instead of received audio data..." << std::endl;
            std::vector<char> silence_buffer(BUFFER_SIZE, 0);

            play_audio(silence_buffer.data(), playback_handle);
        } else {
            // std::cout << "Playing received audio data..." << std::endl;
            play_audio(buffer, playback_handle);
        }
    }

    if (capture_handle) {
        snd_pcm_close(capture_handle);
    }
    snd_pcm_close(playback_handle);
    close(client_socket);

    if (audio_file.is_open()) {
        audio_file.close();
    }

    return 0;
}
