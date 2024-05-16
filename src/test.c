#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sha1_encode(const char *input_string, unsigned char *hash) {
    SHA1((unsigned char *)input_string, strlen(input_string), hash);
}

int main() {
    char input_string[] = "Hello, world!";
    unsigned char hash[SHA_DIGEST_LENGTH];

    sha1_encode(input_string, hash);

    printf("SHA-1 hash of '%s' is: ", input_string);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");

    return 0;
}
