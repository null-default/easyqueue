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
ezq_init_unsafe(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void *(*alloc_fn)(const unsigned long size),
    void (*free_fn)(void * const ptr)
);

static unsigned int EZQ_API
ezq_count_unsafe(const ezq_queue * const p_queue);

static void EZQ_API
ezq_buf_push(struct ezq_buffer * const p_buf, void * const p_item);

static void EZQ_API
ezq_buf_pop(struct ezq_buffer * const p_buf, void ** const pp_item);

static struct ezq_linkedlist_node * EZQ_API
ezq_list_create_node(
    struct ezq_linkedlist * const p_ll,
    void * (*alloc_fn)(const unsigned long size),
    void * const p_item
);

static void EZQ_API
ezq_list_push(
    struct ezq_linkedlist * const p_ll,
    struct ezq_linkedlist_node * const p_newnode
);

static void EZQ_API
ezq_list_pop(
    struct ezq_linkedlist * const p_ll,
    void (*free_fn)(void * const ptr),
    void ** const pp_item
);

static void EZQ_API
ezq_destroy_unsafe(
    ezq_queue * const p_queue,
    void (*item_cleanup_fn)(void * const p_item, void * const p_args),
    void * const p_args
);

ezq_status EZQ_API
ezq_init(
    ezq_queue * const p_queue,
    const unsigned int capacity,
    void *(*alloc_fn)(const unsigned long size),
    void (*free_fn)(void * const ptr)
)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    if (NULL == p_queue)
    {
        estat = EZQ_STATUS_NULL_QUEUE;
        goto done;
    }

    ezq_init_unsafe(p_queue, capacity, alloc_fn, free_fn);
    estat = EZQ_STATUS_SUCCESS;

done:
    return estat;
} /* ezq_init */

ezq_status EZQ_API
ezq_push(ezq_queue * const p_queue, void * const p_item)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    struct ezq_linkedlist_node * p_newnode = NULL;

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
    if (
        p_queue->capacity > 0
        && p_queue->fixed.count + p_queue->dynamic.count >= p_queue->capacity
    )
    {
        estat = EZQ_STATUS_FULL;
        goto done;
    }

    /* If the fixed buffer isn't full, add the item to it. Otherwise, add
     * the item to the linked list.
     * */
    if (p_queue->fixed.count < EZQ_FIXED_BUFFER_CAPACITY)
    {
        ezq_buf_push(&p_queue->fixed, p_item);
    }
    else if (NULL == p_queue->alloc_fn)
    {
        estat = EZQ_STATUS_NO_ALLOC_FN;
        goto done;
    }
    else
    {
        p_newnode = ezq_list_create_node(
            &p_queue->dynamic,
            p_queue->alloc_fn,
            p_item
        );
        if (NULL == p_newnode)
        {
            estat = EZQ_STATUS_ALLOC_FAILURE;
            goto done;
        }

        ezq_list_push(&p_queue->dynamic, p_newnode);
        p_newnode = NULL;
    }

    estat = EZQ_STATUS_SUCCESS;

done:
    return estat;
} /* ezq_push */

ezq_status EZQ_API
ezq_pop(ezq_queue * const p_queue, void ** const pp_item)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    void * p_item = NULL;

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
    if (ezq_count_unsafe(p_queue) < 1)
    {
        estat = EZQ_STATUS_EMPTY;
        goto done;
    }

    ezq_buf_pop(&p_queue->fixed, pp_item);

    /* Move the front item of the linked list into the fixed buffer. */
    if (p_queue->dynamic.count > 0)
    {
        ezq_list_pop(&p_queue->dynamic, p_queue->free_fn, &p_item);

        p_queue->fixed.p_items[
            (p_queue->fixed.front_index - 1) % EZQ_FIXED_BUFFER_CAPACITY
        ] = p_item;
        ++p_queue->fixed.count;
    }

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

    count = ezq_count_unsafe(p_queue);
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

    ezq_destroy_unsafe(p_queue, item_cleanup_fn, p_args);

    estat = EZQ_STATUS_SUCCESS;

done:
    return estat;
} /* ezq_destroy */

static void EZQ_API
ezq_init_unsafe(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void *(*alloc_fn)(const unsigned long size),
    void (*free_fn)(void * const ptr)
)
{
    unsigned int i = 0;
    assert(NULL != p_queue);

    for (i = 0; i < EZQ_FIXED_BUFFER_CAPACITY; ++i)
    {
        p_queue->fixed.p_items[i] = NULL;
    }
    p_queue->fixed.front_index = 0;
    p_queue->fixed.count = 0;

    p_queue->dynamic.p_head = NULL;
    p_queue->dynamic.p_tail = NULL;
    p_queue->dynamic.count = 0;

    p_queue->capacity = capacity;
    p_queue->alloc_fn = alloc_fn;
    p_queue->free_fn = free_fn;
} /* ezq_init_unsafe */

static unsigned int EZQ_API
ezq_count_unsafe(const ezq_queue * const p_queue)
{
    assert(NULL != p_queue);

    return p_queue->fixed.count + p_queue->dynamic.count;
} /* ezq_count_unsafe */

static void EZQ_API
ezq_buf_push(struct ezq_buffer * const p_buf, void * const p_item)
{
    assert(NULL != p_buf);
    assert(NULL != p_item);

    if (p_buf->count < EZQ_FIXED_BUFFER_CAPACITY)
    {
        p_buf->p_items[EZQ_BUF_BACK(p_buf, EZQ_FIXED_BUFFER_CAPACITY)] = p_item;
        ++p_buf->count;
    }
} /* ezq_buf_push */

static void EZQ_API
ezq_buf_pop(struct ezq_buffer * const p_buf, void ** const pp_item)
{
    assert(NULL != p_buf);
    assert(NULL != pp_item);

    *pp_item = p_buf->p_items[p_buf->front_index];
    p_buf->p_items[p_buf->front_index] = NULL;
    p_buf->front_index = (p_buf->front_index + 1) % EZQ_FIXED_BUFFER_CAPACITY;
    --p_buf->count;
} /* ezq_buf_pop */

static struct ezq_linkedlist_node * EZQ_API
ezq_list_create_node(
    struct ezq_linkedlist * const p_ll,
    void * (*alloc_fn)(const unsigned long size),
    void * const p_item
)
{
    struct ezq_linkedlist_node * p_newnode = NULL;

    assert(NULL != p_ll);
    assert(NULL != alloc_fn);
    assert(NULL != p_item);

    p_newnode = alloc_fn(sizeof(*p_newnode));
    if (NULL == p_newnode)
    {
        goto done;
    }
    p_newnode->p_next = NULL;
    p_newnode->p_prev = NULL;
    p_newnode->p_item = p_item;

    done:
        return p_newnode;
} /* ezq_list_create_node */

static void EZQ_API
ezq_list_push(
    struct ezq_linkedlist * const p_ll,
    struct ezq_linkedlist_node * const p_newnode
)
{
    assert(NULL != p_ll);
    assert(NULL != p_newnode);

    if (NULL == p_ll->p_tail)
    {
        p_ll->p_head = p_newnode;
    }
    else
    {
        p_newnode->p_prev = p_ll->p_tail;
        p_ll->p_tail->p_next = p_newnode;
    }
    p_ll->p_tail = p_newnode;
    ++p_ll->count;
} /* ezq_list_push */

static void EZQ_API
ezq_list_pop(
    struct ezq_linkedlist * const p_ll,
    void (*free_fn)(void * const ptr),
    void ** const pp_item
)
{
    struct ezq_linkedlist_node * p_front = NULL;

    assert(NULL != p_ll);
    assert(NULL != pp_item);

    p_front = p_ll->p_head;
    p_ll->p_head->p_prev = NULL;
    p_ll->p_head = p_ll->p_head->p_next;
    --p_ll->count;

    *pp_item = p_front->p_item;
    p_front->p_item = NULL;

    if (NULL != free_fn)
    {
        free_fn(p_front);
    }
    p_front = NULL;
} /* ezq_list_pop */

static void EZQ_API
ezq_destroy_unsafe(
    ezq_queue * const p_queue,
    void (*item_cleanup_fn)(void * const p_item, void * const p_args),
    void * const p_args
)
{
    void *p_item = NULL;

    assert(NULL != p_queue);

    /* Clean up the fixed-size buffer first. */
    while (p_queue->fixed.count > 0)
    {
        ezq_buf_pop(&p_queue->fixed, &p_item);
        if (NULL != item_cleanup_fn)
        {
            item_cleanup_fn(p_item, p_args);
        }
    }

    /* Now clean up the linked list. */
    while (p_queue->dynamic.count > 0)
    {
        ezq_list_pop(&p_queue->dynamic, p_queue->free_fn, &p_item);
        if (NULL != item_cleanup_fn)
        {
            item_cleanup_fn(p_item, p_args);
        }
    }
} /* ezq_destroy_unsafe */
