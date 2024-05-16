#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int *val;
    int num = 42;
    val = &num;
    int **const pptr = &val;

    printf("%d", **pptr);
    return 0;
}
