#include <stdio.h>
#include <stdlib.h>
#include "easyqueue.h"

int main(int argc, char **argv) {

    ezq_init(NULL, 0, NULL, NULL);

    printf("Hello world!\n");

    (void)argc;
    (void)argv;
    return EXIT_SUCCESS;
} /* main */