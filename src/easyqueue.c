#include <assert.h>
#include "easyqueue.h"

#ifndef NULL
 #define NULL ((void *)0)
#endif /* NULL */

typedef struct ezq_buffer ezq_buf;

#define EZQ_BUF_BACK(p_buf, bound) \
    ((((ezq_buf *)(p_buf))->front_index % (bound)) \
    + ((ezq_buf *)(p_buf))->count)

static void EZQ_API
ezq_init_nochecks(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void *(*realloc_fn)(void * ptr, unsigned long size),
    void (*free_fn)(void * const ptr)
);

static unsigned int EZQ_API
ezq_count_nochecks(const ezq_queue * const p_queue);

static void EZQ_API
ezq_push_nochecks(ezq_queue * const p_queue, void * const p_item);

static void EZQ_API
ezq_pop_nochecks(ezq_queue * const p_queue, void ** const pp_item);

static void EZQ_API
ezq_destroy_nochecks(
    ezq_queue * const p_queue,
    void (*item_cleanup_fn)(void * const p_item, void * const p_args),
    void * const p_args
);

ezq_status EZQ_API
ezq_init(
    ezq_queue * const p_queue,
    void *(*realloc_fn)(void * ptr, unsigned long size),
    void (*free_fn)(void * const ptr)
)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    if (NULL == p_queue)
    {
        estat = EZQ_STATUS_NULL_QUEUE;
        goto done;
    }

    ezq_init_nochecks(p_queue, EZQ_FIXED_BUFFER_CAPACITY, realloc_fn, free_fn);
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
    if (NULL == p_item)
    {
        estat = EZQ_STATUS_NULL_ITEM;
        goto done;
    }
    if (p_queue->capacity > 0 && p_queue->fixed.count >= p_queue->capacity)
    {
        estat = EZQ_STATUS_FULL;
        goto done;
    }

    ezq_push_nochecks(p_queue, p_item);

    estat = EZQ_STATUS_SUCCESS;

done:
    return estat;
} /* ezq_push */

ezq_status EZQ_API
ezq_pop(ezq_queue * const p_queue, void ** const pp_item)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    if (NULL == p_queue)
    {
        estat = EZQ_STATUS_NULL_QUEUE;
        goto done;
    }
    if (NULL == pp_item)
    {
        estat = EZQ_STATUS_NULL_OUT;
        goto done;
    }
    if (ezq_count_nochecks(p_queue) < 1)
    {
        estat = EZQ_STATUS_EMPTY;
        goto done;
    }

    ezq_pop_nochecks(p_queue, pp_item);

    estat = EZQ_STATUS_SUCCESS;

done:
    return estat;
} /* ezq_pop */

unsigned int EZQ_API
ezq_count(const ezq_queue * const p_queue, ezq_status * const p_status)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    unsigned int count = 0;

    if (NULL == p_queue)
    {
        estat = EZQ_STATUS_NULL_QUEUE;
        goto done;
    }

    count = ezq_count_nochecks(p_queue);
    estat = EZQ_STATUS_SUCCESS;

done:
    if (NULL != p_status)
    {
        *p_status = estat;
    }
    return count;
} /* ezq_count */

ezq_status EZQ_API
ezq_destroy(
    ezq_queue * const p_queue,
    void (*item_cleanup_fn)(void * const p_item, void * const p_args),
    void * const p_args
)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    if (NULL == p_queue)
    {
        estat = EZQ_STATUS_NULL_QUEUE;
        goto done;
    }

    ezq_destroy_nochecks(p_queue, item_cleanup_fn, p_args);

    estat = EZQ_STATUS_SUCCESS;

done:
    return estat;
} /* ezq_destroy */

static void EZQ_API
ezq_init_nochecks(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void *(*realloc_fn)(void * ptr, unsigned long size),
    void (*free_fn)(void * const ptr)
)
{
    unsigned int i = 0;
    assert(NULL != p_queue);

    for (i = 0; i < EZQ_FIXED_BUFFER_CAPACITY; ++i)
    {
        p_queue->fixed.items[i] = NULL;
    }
    p_queue->fixed.front_index = 0;
    p_queue->fixed.count = 0;

    p_queue->capacity = capacity;
    p_queue->realloc_fn = realloc_fn;
    p_queue->free_fn = free_fn;
} /* ezq_init_nochecks */

static unsigned int EZQ_API
ezq_count_nochecks(const ezq_queue * const p_queue)
{
    unsigned int count = 0;
    assert(NULL != p_queue);

    count = p_queue->fixed.count;

    return count;
} /* ezq_count_nochecks */

static void EZQ_API
ezq_push_nochecks(ezq_queue * const p_queue, void * const p_item)
{
    assert(NULL != p_queue);
    assert(NULL != p_item);

    if (p_queue->fixed.count < EZQ_FIXED_BUFFER_CAPACITY)
    {
        p_queue->fixed.items[
            EZQ_BUF_BACK(&p_queue->fixed, EZQ_FIXED_BUFFER_CAPACITY)
        ] = p_item;
        ++p_queue->fixed.count;
    }
} /* ezq_push_nochecks */

static void EZQ_API
ezq_pop_nochecks(ezq_queue * const p_queue, void ** const pp_item)
{
    assert(NULL != p_queue);
    assert(NULL != pp_item);

    *pp_item = p_queue->fixed.items[p_queue->fixed.front_index];
    p_queue->fixed.items[p_queue->fixed.front_index] = NULL;
    p_queue->fixed.front_index = (p_queue->fixed.front_index + 1)
                                 % EZQ_FIXED_BUFFER_CAPACITY;
    --p_queue->fixed.count;
} /* ezq_pop_nochecks */

static void EZQ_API
ezq_destroy_nochecks(
    ezq_queue * const p_queue,
    void (*item_cleanup_fn)(void * const p_item, void * const p_args),
    void * const p_args
)
{
    void *p_item = NULL;

    assert(NULL != p_queue);

    while (ezq_count_nochecks(p_queue) > 0)
    {
        ezq_pop_nochecks(p_queue, &p_item);
        if (NULL != item_cleanup_fn)
        {
            item_cleanup_fn(p_item, p_args);
        }
    }
} /* ezq_destroy_nochecks */

