# Audio Streaming Project

## Overview

This project consists of two main components: a client and a server for audio streaming over a network. The client captures or reads audio data, sends it to the server, and receives mixed audio data back from the server. The server receives audio data from multiple clients, mixes the audio streams, and sends the mixed audio back to the respective clients.

## Files

- **client.cpp**: Handles audio capture, sending audio data to the server, receiving mixed audio data, and playback.
- **server.cpp**: Manages multiple clients, receives audio data from them, mixes the audio streams, and sends the mixed audio data back to each client.
- **Makefile**: Contains rules for compiling the client and server programs.

## Dependencies

- ALSA (Advanced Linux Sound Architecture) library for audio handling.
- POSIX threads (pthread) for concurrent processing.
- Standard networking libraries for socket programming.

## Building the Project

To build the project, ensure you have the necessary dependencies installed (e.g., ALSA development libraries). Use the provided Makefile for compilation.

1. **Compile the client and server:**

    ```sh
    make all
    ```

    This command will compile `client.cpp` and `server.cpp`, producing executables named `client` and `server`.
    
2. **Clean build artifacts:**

    ```sh
    make clean
    ```

    This command will remove all compiled binaries and object files.

3. **Rebuild the project:**

    ```sh
    make rebuild
    ```

    This command will clean the build artifacts and then recompile the client and server.

## Usage

### Server

Start the server by running the following command:

```sh
./server # The server will start listening for incoming client connections on port 12345.
./client # This will capture audio from the default audio input device and stream it to the server.
```
All clients can capture and playback audio.