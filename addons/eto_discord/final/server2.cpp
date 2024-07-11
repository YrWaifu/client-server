#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#define PORT 12345
#define BUFFER_SIZE 8000
#define MAX_CLIENTS 10

std::map<int, std::vector<char>> clients_buffers;
std::mutex mtx;

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::lock_guard<std::mutex> lock(mtx);
            clients_buffers.erase(client_socket);
            close(client_socket);
            break;
        }

        std::vector<char> mix_buffer(BUFFER_SIZE, 0);

        std::lock_guard<std::mutex> lock(mtx);
        clients_buffers[client_socket] = std::vector<char>(buffer, buffer + bytes_received);

        for (const auto& [client, client_buffer] : clients_buffers) {
            if (client != client_socket) {
                for (size_t i = 0; i < BUFFER_SIZE; ++i) {
                    mix_buffer[i] += client_buffer[i];
                }
            }
        }

        if (send(client_socket, mix_buffer.data(), BUFFER_SIZE, 0) <= 0) {
            std::lock_guard<std::mutex> lock(mtx);
            clients_buffers.erase(client_socket);
            close(client_socket);
            break;
        }
    }
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    std::vector<std::thread> threads;

    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        threads.emplace_back(handle_client, client_socket);
    }

    for (auto& t : threads) {
        t.join();
    }

    close(server_fd);
    return 0;
}
