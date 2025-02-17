#ifndef EASYQUEUE_H
#define EASYQUEUE_H

/* Macro definition to "tag" EZQ API functions. */
#define EZQ_API

#ifndef EZQ_FIXED_BUFFER_CAPACITY
 /*
  * The maximum number of items the queue may hold before resorting to
  * dynamic allocation of further items.
  */
 #define EZQ_FIXED_BUFFER_CAPACITY (32)
#endif /* EZQ_FIXED_BUFFER_CAPACITY */
#if EZQ_FIXED_BUFFER_CAPACITY < 1
 #error "Value of EZQ_FIXED_BUFFER_CAPACITY must be a positive integer"
#endif /* EZQ_FIXED_BUFFER_CAPACITY < 1 */

/*!
 * @struct ezq_buffer
 * @brief Structure encapsulating a simple rotating buffer.
 *
 * @note This structure is exposed in the header to avoid necessitating that
 * instances of it be dynamically allocated, but this structure is not
 * intended to be interacted with directly.
 */
struct ezq_buffer
{
    void *p_items[EZQ_FIXED_BUFFER_CAPACITY]; /* array of items */
    unsigned int front_index; /* index of the front item of the buffer */
    unsigned int count; /* number of items currently in the buffer */
};

/*!
 * @struct ezq_linkedlist_node
 * @brief Structure encapsulating a node for use with \c ezq_linkedlist
 * instances.
 *
 * @note This structure is exposed in the header to avoid necessitating that
 * instances of it be dynamically allocated, but this structure is not
 * intended to be interacted with directly.
 */
struct ezq_linkedlist_node
{
    void * p_item; /* data in the list */
    struct ezq_linkedlist_node * p_next; /* next node in the list */
};

/*!
 * @struct ezq_linkedlist
 * @brief Structure encapsulating a simple singly-linked list implementation
 * for use as the dynamic storage portion of an \c ezq_queue .
 *
 * @note This structure is exposed in the header to avoid necessitating that
 * instances of it be dynamically allocated, but this structure is not
 * intended to be interacted with directly.
 */
struct ezq_linkedlist
{
    struct ezq_linkedlist_node * p_head; /* front node of the list */
    struct ezq_linkedlist_node * p_tail; /* rear node of the list */
    unsigned int count; /* number of nodes in the list */
};

/*!
 * @struct ezq_queue
 * @brief Structure representing a queue with a fixed-size buffer that also
 * makes use of a linked list to support queueing further items.
 *
 * @note This structure is exposed in the header to avoid necessitating that
 * users dynamically allocate instances of it, but instances of this structure
 * are intended to be accessed via the API functions rather than directly.
 */
typedef struct ezq_queue
{
    struct ezq_buffer fixed; /* fixed-size buffer */
    struct ezq_linkedlist dynamic; /* linked list for further items */
    unsigned int capacity; /* optional max number of items; 0 means no limit */

    /* function for dynamically allocating nodes in the linked list */
    void *(*alloc_fn)(const unsigned long size);

    /* function to release dynamically allocated nodes */
    void (*free_fn)(void * const ptr);
} ezq_queue;

/*!
 * @enum ezq_status
 * @brief Constant values used to indicate the success or failure of EZQ API
 * functions and, in the case of a failure, the reason thereof.
 */
typedef enum ezq_status
{
    EZQ_STATUS_SUCCESS = 0x00, /* No errors occurred */
    EZQ_STATUS_NULL_QUEUE, /* Passed pointer to ezq_queue was NULL */
    EZQ_STATUS_NULL_ITEM, /* Passed pointer to data to enqueue was NULL */
    EZQ_STATUS_NULL_OUT, /* Passed pointer to out variable was NULL */

    EZQ_STATUS_FULL, /* Queue was full */
    EZQ_STATUS_EMPTY, /* Queue was empty */

    EZQ_STATUS_NO_ALLOC_FN, /* Dynamic allocation is needed but no func given */
    EZQ_STATUS_NO_FREE_FN, /* No func to free dynamically allocated resource */
    EZQ_STATUS_ALLOC_FAILURE, /* Dynamic allocation attempt failed */

    EZQ_STATUS_UNKNOWN = 0xFF /* Unknown error occurred */
} ezq_status;

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
 * @return \c EZQ_STATUS_SUCCESS if the \c ezq_queue pointed to by \c p_queue
 * is successfully initialized, otherwise an error-specific \c ezq_status
 * value.
 */
ezq_status EZQ_API
ezq_init(
    ezq_queue * const p_queue,
    const unsigned int capacity,
    void *(*alloc_fn)(const unsigned long size),
    void (*free_fn)(void * const ptr)
);

/*!
 * @brief Gets the number of items currently in the queue.
 *
 * @param[in] p_queue Address of an \c ezq_queue to count the items in.
 * @param[out] p_status Optional address of an \c ezq_status in which to
 * place the relevant status code after the operation.
 *
 * @return The number of items currently within the \c ezq_queue pointed
 * to by \c p_queue . If \c p_status was not \c NULL , it will be updated
 * with an \c ezq_status value indicating the success or failure of the
 * underlying operations.
 */
unsigned int EZQ_API
ezq_count(const ezq_queue * const p_queue, ezq_status * const p_status);

/*!
 * @brief Places \c p_item at the tail end of a queue.
 *
 * @param[in,out] p_queue Address of an \c ezq_queue in which to place the
 * item.
 * @param[in] p_item Pointer to arbitrary data to place on the queue. May
 * not be \c NULL .
 *
 * @return \c EZQ_STATUS_SUCCESS if \c p_item is successfully placed at the
 * end of the \c ezq_queue pointed to by \c p_queue, otherwise an
 * error-specific \c ezq_status value.
 */
ezq_status EZQ_API
ezq_push(ezq_queue * const p_queue, void * const p_item);

/*!
 * @brief Retrieves the front item of the queue and places it in the location
 * pointed to by \c pp_item .
 *
 * @param[in,out] p_queue Address of an \c ezq_queue to retrieve the front
 * item of.
 * @param[out] pp_item Address in which to store the item retrieved from
 * the queue.
 *
 * @return \c EZQ_STATUS_SUCCESS if the front item of the \c ezq_queue pointed
 * to by \c p_queue is retrieved and placed in the location pointed to by
 * \c pp_item , otherwise an error-specific \c ezq_status value.
 */
ezq_status EZQ_API
ezq_pop(ezq_queue * const p_queue, void ** const pp_item);

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
 * @return \c EZQ_STATUS_SUCCESS if the \c ezq_queue pointed to by \c p_queue
 * is successfully cleared, otherwise an error-specific \c ezq_status value.
 */
ezq_status EZQ_API
ezq_destroy(
    ezq_queue * const p_queue,
    void (*item_cleanup_fn)(void *p_item, void *p_args),
    void * const p_args
);

#endif /* EASYQUEUE_H */
