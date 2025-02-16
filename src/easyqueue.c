#include <assert.h>
#include "easyqueue.h"

#ifndef NULL
 #define NULL ((void *)0)
#endif /* NULL */

static void EZQ_API
ezq_init_nochecks(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void * (*alloc_fn)(const unsigned long),
    void (*free_fn)(void * const ptr)
);

static void EZQ_API
ezq_push_fixed_nochecks(ezq_queue * const p_queue, void * const p_item);

static ezq_status EZQ_API
ezq_push_linked_nochecks(ezq_queue * const p_queue, void * const p_item);

ezq_status EZQ_API
ezq_init(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void * (*alloc_fn)(const unsigned long size),
    void (*free_fn)(void * const ptr)
)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    if (NULL == p_queue)
    {
        estat = EZQ_STATUS_NULL_QUEUE;
        goto done;
    }

    ezq_init_nochecks(p_queue, capacity, alloc_fn, free_fn);
    estat = EZQ_STATUS_SUCCESS;

done:
    return estat;
} /* ezq_init */

ezq_status EZQ_API
ezq_push(ezq_queue * const p_queue, void * const p_item)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    if (NULL == p_queue)
    {
        estat = EZQ_STATUS_NULL_QUEUE;
        goto done;
    }
    if (p_queue->capacity == 0 && p_queue->size >= p_queue->capacity)
    {
        estat = EZQ_STATUS_FULL;
        goto done;
    }

    if (p_queue->size < EZQ_FIXED_BUFFER_CAPACITY)
    {
        ezq_push_fixed_nochecks(p_queue, p_item);
        estat = EZQ_STATUS_SUCCESS;
    }
    else
    {
        estat = ezq_push_linked_nochecks(p_queue, p_item);
    }

done:
    return estat;
} /* ezq_push */

static void EZQ_API
ezq_init_nochecks(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void * (*alloc_fn)(const unsigned long),
    void (*free_fn)(void * const ptr)
)
{
    assert(NULL != p_queue);

    p_queue->p_tail = NULL;
    p_queue->size = 0;
    p_queue->capacity = capacity;
    p_queue->alloc_fn = alloc_fn;
    p_queue->free_fn = free_fn;
} /* ezq_init_nochecks */

static void EZQ_API
ezq_push_fixed_nochecks(ezq_queue * const p_queue, void * const p_item)
{
    assert(NULL != p_queue);
    assert(p_queue->size < EZQ_FIXED_BUFFER_CAPACITY);
    assert(p_queue->capacity > 0 ? (p_queue->size < p_queue->capacity) : 1);
    assert(NULL != p_item);

    p_queue->fixed_buffer[p_queue->size] = p_item;
    ++p_queue->size;
} /* ezq_push_fixed_nochecks */

static ezq_status EZQ_API
ezq_push_linked_nochecks(ezq_queue * const p_queue, void * const p_item)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    struct ezq_queue_node * p_newnode = NULL;

    assert(NULL != p_queue);
    assert(NULL != p_queue->alloc_fn);
    assert(p_queue->size >= EZQ_FIXED_BUFFER_CAPACITY);
    assert(p_queue->capacity > 0 ? (p_queue->size < p_queue->capacity) : 1);
    assert(NULL != p_item);

    /* Allocate a new node */
    p_newnode = p_queue->alloc_fn(sizeof(*p_queue->p_tail));
    if (NULL == p_newnode)
    {
        estat = EZQ_STATUS_ALLOC_FAILURE;
        goto done;
    }
    p_newnode->p_data = p_item;

    /* The new node is now the tail end of the linked list portion */
    if (NULL == p_queue->p_tail)
    {
        p_newnode->p_prev = NULL;
        p_queue->p_tail = p_newnode;
        p_queue->p_head = p_newnode; /* New node is also head of the list */
    }
    else
    {
        p_newnode->p_prev = p_queue->p_tail;
        p_queue->p_tail = p_newnode;
    }

    p_newnode = NULL;
    ++p_queue->size;

done:
    /* Shouldn't be possible, but kept here as future-proofing safety measure */
    if (NULL != p_newnode)
    {
        if (NULL != p_queue->free_fn)
        {
            p_queue->free_fn(p_newnode);
            p_newnode = NULL;
        }
    }
    return estat;
} /* ezq_push_linked_nochecks */
