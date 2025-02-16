#ifndef EASYQUEUE_H
#define EASYQUEUE_H

#define EZQ_API /* Macro definition to "tag" EZQ API functions */

#ifndef EZQ_FIXED_BUFFER_CAPACITY
 #define EZQ_FIXED_BUFFER_CAPACITY (16)
#endif /* EZQ_FIXED_BUFFER_CAPACITY */

struct ezq_queue_node
{
    void * p_data;
    struct ezq_queue_node * p_prev;
};

typedef struct ezq_queue
{
    void *fixed_buffer[EZQ_FIXED_BUFFER_CAPACITY];
    struct ezq_queue_node * p_head;
    struct ezq_queue_node * p_tail;
    unsigned long size;
    unsigned long capacity;
    void *(*alloc_fn)(const unsigned long size);
    void (*free_fn)(void * const ptr);
} ezq_queue;

typedef enum ezq_status
{
    EZQ_STATUS_SUCCESS = 0x00, /* No errors occurred */
    EZQ_STATUS_NULL_QUEUE, /* Passed pointer to ezq_queue was NULL */

    EZQ_STATUS_FULL, /* Queue was full */

    EZQ_STATUS_ALLOC_FAILURE, /* Dynamic allocation attempt failed */

    EZQ_STATUS_UNKNOWN = 0xFF /* Unknown error occurred */
} ezq_status;

ezq_status EZQ_API ezq_init(
    ezq_queue * const p_queue,
    const unsigned long capacity,
    void * (*alloc_fn)(const unsigned long size),
    void (*free_fn)(void * const ptr)
);

ezq_status EZQ_API ezq_push(
    ezq_queue * const p_queue,
    void * const p_item
);

#endif /* EASYQUEUE_H */
