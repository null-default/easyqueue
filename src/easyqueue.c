#include <assert.h>
#include "easyqueue.h"

#ifndef NULL
 #define NULL ((void *)0)
#endif /* NULL */

typedef struct ezq_buffer ezq_buf; /* shorthand name for convenience */

/* Gets the next available index of the fixed size buffer. */
#define EZQ_BUF_BACK(p_buf, bound) \
    ((((ezq_buf *)(p_buf))->front_index % (bound)) \
    + ((ezq_buf *)(p_buf))->count)

/*!
 * @brief Initializes an \c ezq_queue structure such that it contains no
 * items.
 *
 * @param[in,out] p_queue Address of an \c ezq_queue to initialize.
 * @param[in] capacity Maximum number of items that may be placed in the
 * queue ( \c 0 for no limit).
 * @param[in] alloc_fn Function used to allocate memory needed to store
 * more items when the fixed size buffer is full.
 * @param[in] free_fn Function used to release memory allocated for items
 * when the fixed size buffer is full.
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static void EZQ_API
ezq_init_unsafe(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void *(*alloc_fn)(const unsigned long size),
    void (*free_fn)(void * const ptr)
);

/*!
 * @brief Gets the number of items currently in the queue.
 *
 * @param[in] p_queue Address of an \c ezq_queue to count the items in.
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static unsigned int EZQ_API
ezq_count_unsafe(const ezq_queue * const p_queue);

/*!
 * @brief Places \c p_item into the next available location within the
 * buffer pointed to by \c p_buf .
 *
 * @param[in,out] p_buf Address of an \c ezq_buffer to push \c p_item onto.
 * @param[in] p_item Pointer to store in the buffer.
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static void EZQ_API
ezq_buf_push(struct ezq_buffer * const p_buf, void * const p_item);

/*!
 * @brief Removes the front item from the buffer pointed to by \c p_buf
 * and places the item in the location pointed to by \c pp_item .
 *
 * @param[in,out] p_buf Address of an \c ezq_buffer to pop an item from.
 * @param[out] pp_item Address in which to store the popped item.
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static void EZQ_API
ezq_buf_pop(struct ezq_buffer * const p_buf, void ** const pp_item);

/*!
 * @brief Dynamically allocates an \c ezq_linkedlist_node and ensures
 * its fields are zero-initialized.
 *
 * @param[in] alloc_fn Function used to allocate memory for the new
 * \c ezq_linkedlist_node .
 * @param[in] p_item Pointer to store in the resulting
 * \c ezq_linkedlist_node .
 *
 * @return The address of a dynamically allocated \c ezq_linkedlist_node
 * if successful, otherwise \c NULL .
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static struct ezq_linkedlist_node * EZQ_API
ezq_list_create_node(
    void * (*alloc_fn)(const unsigned long size),
    void * const p_item
);

/*!
 * @brief Appends the node pointed to by \c p_newnode to the end of the
 * linked list pointed to by \c p_ll .
 *
 * @param[in,out] p_ll Address of an \c ezq_linkedlist to append to.
 * @param[in] p_node Address of an \c ezq_linkedlist_node to append to
 * the linked list.
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static void EZQ_API
ezq_list_push(
    struct ezq_linkedlist * const p_ll,
    struct ezq_linkedlist_node * const p_node
);

/*!
 * @brief Removes the front node of the linked list pointed to by \c p_ll
 * and places the node's data into the location pointed to by \c pp_item .
 *
 * @param[in,out] p_ll Address of an \c ezq_linkedlist to retrieve the
 * front node of.
 * @param[in] free_fn Function used to release memory allocated for the
 * node being removed.
 * @param[out] pp_item Address in which to store the item that was in the
 * retrieved node.
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static void EZQ_API
ezq_list_pop(
    struct ezq_linkedlist * const p_ll,
    void (*free_fn)(void * const ptr),
    void ** const pp_item
);

/*!
 * @brief Clears the queue, performing any necessary cleanup.
 *
 * @param[in,out] p_queue Address of an \c ezq_queue structure to destroy.
 * @param[in] item_cleanup_fn Optional function that will be invoked each
 * remaining item in the queue, in case those items require additional
 * cleanup handling.
 * @param[in] p_args Optional pointer to an arbitrary structure that may
 * contain any additional resources necessary for cleanup of remaining items.
 *
 * @note This function performs no safety checks (e.g. checks for
 * \c NULL ) on its passed arguments in release builds.
 */
static void EZQ_API
ezq_destroy_unsafe(
    ezq_queue * const p_queue,
    void (*item_cleanup_fn)(void *p_item, void *p_args),
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
    if (NULL == alloc_fn)
    {
        estat = EZQ_STATUS_NO_ALLOC_FN;
        goto done;
    }
    if (NULL == free_fn)
    {
        estat = EZQ_STATUS_NO_FREE_FN;
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
        p_newnode = ezq_list_create_node(p_queue->alloc_fn, p_item);
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
    if (p_queue->fixed.count < 1)
    {
        estat = EZQ_STATUS_EMPTY;
        goto done;
    }
    if (p_queue->dynamic.count > 0 && NULL == p_queue->free_fn)
    {
        estat = EZQ_STATUS_NO_FREE_FN;
        goto done;
    }

    ezq_buf_pop(&p_queue->fixed, pp_item);

    /* Move front item of the linked list to the back of the fixed buffer. */
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
    void (*item_cleanup_fn)(void *p_item, void *p_args),
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
    void * (*alloc_fn)(const unsigned long size),
    void * const p_item
)
{
    struct ezq_linkedlist_node * p_newnode = NULL;

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
    struct ezq_linkedlist_node * const p_node
)
{
    assert(NULL != p_ll);
    assert(NULL != p_node);

    if (NULL == p_ll->p_tail)
    {
        p_ll->p_head = p_node;
    }
    else
    {
        p_node->p_prev = p_ll->p_tail;
        p_ll->p_tail->p_next = p_node;
    }
    p_ll->p_tail = p_node;
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
    void (*item_cleanup_fn)(void * p_item, void *p_args),
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
