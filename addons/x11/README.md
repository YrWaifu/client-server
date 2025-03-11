
# X11 Clipboard Project

## Overview

This project consists of simple programs written in C and C++ to transfer files, text messages, or images using the X11 clipboard. It allows users to copy and paste various types of data seamlessly across applications that support the X11 clipboard protocol.

## About Files

### `copy_file.c`
A C program to copy files to the clipboard.

```sh
gcc -o copy_file copy_file.c -lX11
./copy_file <path_to_file> # Copies the specified file to the clipboard. You can then paste it using Ctrl+V (works with GNOME and MATE).
```

### `copy_png.cpp`
A C++ program to copy PNG images to the clipboard.

```sh
g++ -o copy_png copy_png.cpp -lX11
./copy_png <path_to_png_file> # Copies the specified PNG image to the clipboard.
```

### `paste.c`
A C program to retrieve and print the current clipboard content.

```sh
gcc -o paste paste.c -lX11
./paste # Retrieves the current clipboard content and prints it to the standard output.
```