#include <stdio.h>
#include <stdlib.h>
#include "easyqueue.h"

#define NUM_PUSH_ITEMS (32)
#define NUM_POP_ITEMS  (32)

static int g_num_allocs = 0;
static int g_num_frees = 0 ;

static void * my_malloc(const size_t size) {
    void * ptr = NULL;

    ptr = malloc(size);
    if (NULL == ptr)
    {
        printf("Failed to allocate %lu bytes!\n", size);
        goto done;
    }
    ++g_num_allocs;
    printf("Allocation %d\n", g_num_allocs);

done:
    return ptr;
} /* my_malloc */

static void my_free(void * const ptr)
{
    free(ptr);
    ++g_num_frees;
    printf("Free #%d\n", g_num_frees);
}

int main(int argc, char **argv)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    ezq_queue queue = { 0 };
    int items[NUM_PUSH_ITEMS] = { 0 };
    int *popped = NULL;
    unsigned int i = 0;

    estat = ezq_init(&queue, 0, my_malloc, my_free);

#ifndef NDEBUG
    printf("Hello world!\n");
#endif

    for (i = 0; i < NUM_PUSH_ITEMS; ++i)
    {
        items[i] = (int)i;
        estat = ezq_push(&queue, &items[i]);
        if (EZQ_STATUS_SUCCESS != estat)
        {
            printf("ezq_push() failed with status==%u\n", estat);
            break;
        }
    }
    printf("Pushed %u/%u items\n", i, NUM_PUSH_ITEMS);

    /*for (i = 0; i < NUM_POP_ITEMS; ++i)
    {
        estat = ezq_pop(&queue, (void **)&popped);
        if (EZQ_STATUS_SUCCESS != estat)
        {
            printf("ezq_pop() failed with status==%u\n", estat);
            break;
        }
        printf("  Popped value: %d\n", *popped);
    }
    printf("Popped %u/%u items\n", i, NUM_POP_ITEMS);*/
    (void)popped;

    estat = ezq_destroy(&queue, NULL, NULL);

    (void)argc;
    (void)argv;
    return estat == EZQ_STATUS_SUCCESS ? EXIT_SUCCESS : (int)estat;
} /* main */