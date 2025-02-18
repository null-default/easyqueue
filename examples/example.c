#include <stdio.h>
#include <stdlib.h>
#include "easyqueue.h"

#define NUM_PUSH_ITEMS (EZQ_FIXED_BUFFER_CAPACITY * 2)
#define NUM_POP_ITEMS  (NUM_PUSH_ITEMS / 2)

#define PRINT_FAILURE(func_name, status) \
    do { \
        printf("[!] " #func_name "() failed with status==%u\n", status); \
    } while (0)

/*!
 * @brief Simple cleanup routine intended to be passed to \c ezq_destroy()
 * if dynamically items are still in the queue at time of destruction.
 *
 * @param[in,out] p_item Dynamically allocated address to free.
 * @param[out] p_count Address of an integer to increment, to count the
 * number of times this function is called.
 */
static void
my_cleanup_fn(void * const p_item, void * const p_count);

/*!
 * @brief Demonstrates a simple usage of an \e ezq_queue (and it's
 * associated API) with stack-allocated items.
 *
 * @return \c EZQ_STATUS_SUCCESS if all EZQ API calls are successful,
 * otherwise an error-specific \c ezq_status value.
 */
static ezq_status
stack_items_example(void);

/*!
 * @brief Demonstrates a simple usage of an \e ezq_queue (and it's
 * associated API) with heap-allocated items.
 *
 * @return \c EZQ_STATUS_SUCCESS if all EZQ API calls are successful,
 * otherwise an error-specific \c ezq_status value.
 */
static ezq_status
heap_items_example(void);

int
main(int argc, char **argv)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    printf(
        "[+] Demonstrating using a queue with stack-allocated items\n"
    );
    estat = stack_items_example();
    if (EZQ_STATUS_SUCCESS != estat)
    {
        goto done;
    }

    printf(
        "\n[+] Demonstrating using a queue with heap-allocated items\n"
    );
    estat = heap_items_example();
    if (EZQ_STATUS_SUCCESS != estat)
    {
        goto done;
    }

done:
    (void)argc;
    (void)argv;
    return estat == EZQ_STATUS_SUCCESS ? EXIT_SUCCESS : (int)estat;
} /* main */

static void
my_cleanup_fn(void * const p_item, void * const p_count)
{
    /* Items should have been allocated memory via malloc(), so all that's
     * needed is to call free() on them.
     */
    free(p_item);

    /* Arbitrary data can also be passed if more is needed during the
     * cleanup routine. This is normally for supporting cleanup of more
     * complex structures, but we can use this functionality for other
     * things (like, in this case, counting the number of times this
     * function is invoked).
     */
    ++(*(unsigned int *)p_count);
} /* my_cleanup_fn */

static ezq_status
stack_items_example(void)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    ezq_queue queue = { 0 };
    unsigned int items[NUM_PUSH_ITEMS] = { 0 };
    unsigned int *popped = NULL;
    unsigned int i = 0;

    /* Initialize the queue. */
    estat = ezq_init(&queue, 0, malloc, free);
    if (EZQ_STATUS_SUCCESS != estat)
    {
        PRINT_FAILURE(ezq_init, estat);
        goto done;
    }

    printf("[+]   Pushing onto fixed-size portion of queue...\n");
    for (i = 0; i < NUM_PUSH_ITEMS; ++i)
    {
        /* Push an item onto the queue. A copy of the address provided to
         * ezq_push() is what is actually stored.
         * */
        items[i] = i + 1;
        estat = ezq_push(&queue, &items[i]);
        if (EZQ_STATUS_SUCCESS != estat)
        {
            PRINT_FAILURE(ezq_push, estat);
            goto done;
        }
        printf("[*]     Pushed value %u\n", items[i]);

        /* When the fixed-size portion of the queue is filled, subsequent
         * pushes will begin storing items in an underlying linked list.
         * */
        if (EZQ_FIXED_BUFFER_CAPACITY == i + 1 && i + 1 < NUM_PUSH_ITEMS)
        {
            printf(
                "[+]   Now pushing onto dynamic portion of queue...\n"
            );
        }
    }
    printf("[+]   Pushed %u/%u items\n", i, NUM_PUSH_ITEMS);

    /* Pop items from the queue. */
    printf(
        "[+]   Popping items from queue; items will be moved from the "
        "dynamic portion into the fixed-size portion as space is available\n"
    );
    for (i = 0; i < NUM_POP_ITEMS; ++i)
    {
        estat = ezq_pop(&queue, (void **)&popped);
        if (EZQ_STATUS_SUCCESS != estat)
        {
            PRINT_FAILURE(ezq_pop, estat);
            goto done;
        }
        printf("[*]     Popped value: %d\n", *popped);
    }
    printf("[+]   Popped %u/%u items\n", i, NUM_POP_ITEMS);

    /* Tear down the queue. Any remaining items will be implicitly popped.
     * Since the items in the queue do not require any kind of cleanup
     * routine, no function is passed to ezq_destroy().
     */
    printf("[+]   Tearing down queue\n");
    estat = ezq_destroy(&queue, NULL, NULL);
    if (EZQ_STATUS_SUCCESS != estat)
    {
        PRINT_FAILURE(ezq_destroy, estat);
    }

done:
    return estat;
} /* stack_items_example */

static ezq_status
heap_items_example(void)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    ezq_queue queue = { 0 };
    unsigned int *p_item = NULL;
    unsigned int *popped = NULL;
    unsigned int i = 0;

    /* Initialize the queue. We have to provide dynamic allocation (and
     * free) functions in case the underlying fixed-suze buffer is filled.
     * */
    estat = ezq_init(&queue, 0, malloc, free);
    if (EZQ_STATUS_SUCCESS != estat)
    {
        PRINT_FAILURE(ezq_init, estat);
        goto done;
    }

    printf("[+]   Pushing onto fixed-size portion of queue...\n");
    for (i = 0; i < NUM_PUSH_ITEMS; ++i)
    {
        /* Here, we will dynamically allocate memory for every single item
         * that is pushed onto the queue. These items will need to be freed
         * after being popped from the queue.
         */
        p_item = malloc(sizeof(int));
        if (NULL == p_item)
        {
            printf("[!] malloc() failure\n");
            goto done;
        }
        *p_item = i + 1;

        /* Push items onto the queue. A copy of the address provided to
         * ezq_push() is what is actually stored.
         * */
        estat = ezq_push(&queue, p_item);
        if (EZQ_STATUS_SUCCESS != estat)
        {
            PRINT_FAILURE(ezq_push, estat);
            goto done;
        }
        printf("[*]     Pushed value %u\n", *p_item);

        /* IMPORTANT: It is up to the caller to properly enforce memory
         * ownership after an item is pushed onto the queue.
         * */
        p_item = NULL;

        /* When the fixed-size portion of the queue is filled, subsequent
         * pushes will begin storing items in an underlying linked list.
         * */
        if (EZQ_FIXED_BUFFER_CAPACITY == i + 1 && i + 1 < NUM_PUSH_ITEMS)
        {
            printf(
                "[+]   Now pushing onto dynamic portion of queue...\n"
            );
        }
    }
    printf("[+]   Pushed %u/%u items\n", i, NUM_PUSH_ITEMS);

    /* Pop items from the queue. */
    printf(
        "[+]   Popping items from queue; items will be moved from the "
        "dynamic portion into the fixed-size portion as space is available\n"
    );
    for (i = 0; i < NUM_POP_ITEMS; ++i)
    {
        estat = ezq_pop(&queue, (void **)&popped);
        if (EZQ_STATUS_SUCCESS != estat)
        {
            PRINT_FAILURE(ezq_pop, estat);
            goto done;
        }
        printf("[*]     Popped value: %d\n", *popped);

        /* Since the pushed items were dynamically allocated, they should be
         * appropriately freed.
         */
        free(popped);
        popped = NULL;
    }
    printf("[+]   Popped %u/%u items\n", i, NUM_POP_ITEMS);

    /* Tear down the queue. Any remaining items will be implicitly popped.
     * Note how this time we pass a cleanup function to ezq_destroy().
     */
    printf("[+]   Tearing down queue\n");
    i = 0;
    estat = ezq_destroy(&queue, my_cleanup_fn, &i);
    if (EZQ_STATUS_SUCCESS != estat)
    {
        PRINT_FAILURE(ezq_destroy, estat);
        goto done;
    }
    printf("[+]     my_cleanup_fn() was called %u times\n", i);

done:
    return estat;
} /* heap_items_example */