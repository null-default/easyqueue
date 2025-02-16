#include <stdio.h>
#include <stdlib.h>
#include "easyqueue.h"

int main(int argc, char **argv)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    ezq_queue queue = { 0 };

    estat = ezq_init(&queue, NULL, NULL);

#ifndef NDEBUG
    printf("Hello world!\n");
#endif

    estat = ezq_destroy(&queue, NULL, NULL);

    (void)argc;
    (void)argv;
    return estat == EZQ_STATUS_SUCCESS ? EXIT_SUCCESS : (int)estat;
} /* main */