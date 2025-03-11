// record_audio.cpp
#include <alsa/asoundlib.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#define BUFFER_SIZE 4096
#define SAMPLE_RATE 44100
#define CHANNELS 1
#define DURATION_SECONDS 10

void generateUniqueFilename(std::string &filename) {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "audio_" << std::put_time(std::localtime(&now_time), "%Y%m%d_%H%M%S") << ".raw";
    filename = ss.str();
}

void setupAlsa(snd_pcm_t **captureHandle) {
    snd_pcm_open(captureHandle, "default", SND_PCM_STREAM_CAPTURE, 0);
    snd_pcm_set_params(*captureHandle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, CHANNELS,
                       SAMPLE_RATE, 1, 500000);
}

int main() {
    snd_pcm_t *captureHandle;
    char buffer[BUFFER_SIZE];
    std::string filename;

    // Generate unique filename
    generateUniqueFilename(filename);

    // Setup ALSA for recording
    setupAlsa(&captureHandle);

    std::ofstream outputFile(filename, std::ios::binary);
    if (!outputFile) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return 1;
    }

    int frames_to_capture = SAMPLE_RATE * DURATION_SECONDS / (BUFFER_SIZE / 2);

    std::cout << "Recording audio to " << filename << " for " << DURATION_SECONDS << " seconds..."
              << std::endl;

    for (int i = 0; i < frames_to_capture; ++i) {
        snd_pcm_readi(captureHandle, buffer, BUFFER_SIZE / 2);
        outputFile.write(buffer, BUFFER_SIZE);
    }

    std::cout << "Recording complete." << std::endl;

    outputFile.close();
    snd_pcm_close(captureHandle);

    return 0;
}
