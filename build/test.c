// reverse.c
#include <stdio.h>
#include <string.h>

int main() {
    char message[100];
    while (1) {
        if (fgets(message, sizeof(message), stdin)) {
            message[strcspn(message, "\n")] = 0;

            int length = strlen(message);
            for (int i = length - 1; i >= 0; i--) {
                putchar(message[i]);
            }
            putchar('\n');
            fflush(stdout);
        }
    }
    return 0;
}
