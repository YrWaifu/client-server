// play_audio.cpp
#include <alsa/asoundlib.h>

#include <fstream>
#include <iostream>
#include <string>

#define BUFFER_SIZE 4096
#define SAMPLE_RATE 44100
#define CHANNELS 2

void setupAlsa(snd_pcm_t **playbackHandle) {
    snd_pcm_open(playbackHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_set_params(*playbackHandle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, CHANNELS,
                       SAMPLE_RATE, 1, 500000);
}

void playAudioFile(const std::string &filename, snd_pcm_t *playbackHandle) {
    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return;
    }

    char buffer[BUFFER_SIZE];
    while (inputFile.read(buffer, BUFFER_SIZE)) {
        snd_pcm_writei(playbackHandle, buffer, BUFFER_SIZE / 4);
    }

    inputFile.close();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    snd_pcm_t *playbackHandle;

    // Setup ALSA for playback
    setupAlsa(&playbackHandle);

    std::cout << "Playing audio from " << filename << "..." << std::endl;
    playAudioFile(filename, playbackHandle);
    std::cout << "Playback complete." << std::endl;

    snd_pcm_close(playbackHandle);

    return 0;
}
