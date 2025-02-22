#include <stdio.h>
#include <unity/unity.h>
#include "easyqueue.h"

#define CUSTOM_ALLOC_STACK_SIZE (16)

struct alloc_stack
{
    void *return_stack[CUSTOM_ALLOC_STACK_SIZE];
    int top_index;
} g_alloc_stack;

/*!
 * @brief Sets the global dummy allocation stack to default values.
 *
 * @note This function's implementation (regardless of what it actually does)
 * is required by the Unity test framework.
 */
void setUp(void)
{
    unsigned int i = 0;

    for (i = 0; i < CUSTOM_ALLOC_STACK_SIZE; ++i)
    {
        g_alloc_stack.return_stack[i] = NULL;
    }
    g_alloc_stack.top_index = -1;
}

void tearDown(void) { } /* UNUSED; required definition for Unity tests */

/*!
 * @brief Pops the address at the top of global dummy allocation stack and
 * returns it.
 *
 * @param[in] size UNUSED
 *
 * @return The address at the top of the global dummy allocation stack. If
 * no return values have been set on the stack, \c NULL is returned.
 */
static void *
custom_alloc_fn(const size_t size)
{
    void *p_addr = NULL;

    if (g_alloc_stack.top_index < 0)
    {
        goto done;
    }

    p_addr = g_alloc_stack.return_stack[g_alloc_stack.top_index];
    g_alloc_stack.return_stack[g_alloc_stack.top_index] = NULL;
    if (g_alloc_stack.top_index > 0)
    {
        --g_alloc_stack.top_index;
    }

done:
    (void)size;
    return p_addr;
} /* custom_alloc_fn */

/*!
 * @brief Pushes \c ptr to the top of the global dummy allocation stack
 * such that it will be returned by the next call to \c alloc_fn_custom() .
 *
 * @param[in] ptr The address to push onto the stack.
 */
static void
custom_alloc_fn_push(void * const ptr)
{
    if (g_alloc_stack.top_index + 1 < CUSTOM_ALLOC_STACK_SIZE)
    {
        ++g_alloc_stack.top_index;
    }
    g_alloc_stack.return_stack[g_alloc_stack.top_index] = ptr;
} /* custom_alloc_fn_push */

/*!
 * @brief Does nothing.
 *
 * @note This function mirrors the signature of the standard library's
 * \c free() function.
 *
 * @param[in] ptr UNUSED
 */
static void
custom_free_fn(void * const ptr)
{
    (void)ptr;
} /* custom_free_fn */

/*!
 * @brief Test that \c ezq_init succeeds when provided standard valid
 * arguments.
 */
static void
test__ezq_init__standard__success(void)
{
    const unsigned int TEST_CAPACITY = 100;
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    ezq_queue queue = { 0 };
    int i = 0;

    for (i = 0; i < sizeof(queue); ++i)
    {
        ((unsigned char *)&queue)[i] = 0xFF;
    }

    estat = ezq_init(&queue, TEST_CAPACITY, custom_alloc_fn, custom_free_fn);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_SUCCESS, estat);
    for (i = 0; i < EZQ_FIXED_BUFFER_CAPACITY; ++i)
    {
        TEST_ASSERT_NULL(queue.fixed.p_items[i]);
    }
    TEST_ASSERT_EQUAL_UINT32(0, queue.fixed.front_index);
    TEST_ASSERT_EQUAL_UINT32(0, queue.fixed.count);

    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
    TEST_ASSERT_EQUAL(0, queue.dynamic.count);

    TEST_ASSERT_EQUAL(TEST_CAPACITY, queue.capacity);
    TEST_ASSERT_EQUAL_PTR(queue.alloc_fn, custom_alloc_fn);
    TEST_ASSERT_EQUAL_PTR(queue.free_fn, custom_free_fn);
} /* test__ezq_init__standard__success */

/*!
 * @brief Test that \c ezq_init properly fails when passed a \c NULL
 * \c ezq_queue* .
 */
static void
test__ezq_init__null_queue__failure(void)
{
    const unsigned int TEST_CAPACITY = 100;
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    estat = ezq_init(NULL, TEST_CAPACITY, custom_alloc_fn, custom_free_fn);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NULL_QUEUE, estat);
} /* test__ezq_init__null_queue__failure */

/*!
 * @brief Tests that \c ezq_push properly pushes to the underlying
 * fixed-size buffer when given standard valid arguments.
 */
static void
test__ezq_push__buf__success(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Set any initial state. */
    queue.fixed.count = 0;
    queue.fixed.front_index = 0;
    queue.fixed.p_items[0] = NULL;
    queue.dynamic.count = 0;
    queue.dynamic.p_head = NULL;
    queue.dynamic.p_tail = NULL;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(&queue, p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_SUCCESS, estat);
    TEST_ASSERT_EQUAL_PTR(p_item, queue.fixed.p_items[0]);
    TEST_ASSERT_EQUAL_UINT32(1, queue.fixed.count);
    TEST_ASSERT_EQUAL_UINT32(0, queue.fixed.front_index);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_UINT32(0, queue.dynamic.count);
    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
} /* test__ezq_push__buf__success */

/*!
 * @brief Tests that \c ezq_push properly pushes to the underlying linked
 * list when the fixed size buffer is full.
 */
static void
test__ezq_push__list__success(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;
    struct ezq_linkedlist_node node = { NULL, NULL };

    /* Set any initial state. */
    custom_alloc_fn_push(&node);
    queue.alloc_fn = custom_alloc_fn;
    queue.fixed.count = EZQ_FIXED_BUFFER_CAPACITY;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(&queue, p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_SUCCESS, estat);
    TEST_ASSERT_EQUAL_PTR(&node, queue.dynamic.p_head);
    TEST_ASSERT_EQUAL_PTR(&node, queue.dynamic.p_tail);
    TEST_ASSERT_EQUAL_PTR(p_item, node.p_item);
    TEST_ASSERT_NULL(node.p_next);
} /* test__ezq_push__list__success */

/*!
 * @brief Tests that \c ezq_push fails when the passed \c ezq_queue pointer
 * is \c NULL .
 */
static void
test__ezq_push__null_queue__failure(void)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(NULL, p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NULL_QUEUE, estat);
} /* test__ezq_push__null_queue__failure */

/*!
 * @brief Tests that \c ezq_push fails when the passed item is \c NULL .
 */
static void
test__ezq_push__null_item__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    /* Set any initial state. */
    queue.fixed.count = 0;
    queue.fixed.p_items[0] = NULL;
    queue.dynamic.count = 0;
    queue.dynamic.p_head = NULL;
    queue.dynamic.p_tail = NULL;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(&queue, NULL);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NULL_ITEM, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_UINT32(0, queue.fixed.count);
    TEST_ASSERT_EQUAL_UINT32(0, queue.dynamic.count);
    TEST_ASSERT_NULL(queue.fixed.p_items[0]);
    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
} /* test__ezq_push__null_item__failure */

/*!
 * @brief Tests that \c ezq_push fails when the queue has a non-zero
 * capacity and the underlying fixed-size buffer has reached that
 * capacity.
 */
static void
test__ezq_push__capacity_full_buf__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Set any initial state. */
    queue.capacity = 1;
    queue.fixed.count = 1;
    queue.fixed.p_items[0] = NULL;
    queue.dynamic.count = 0;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(&queue, p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_FULL, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_UINT32(1, queue.fixed.count);
    TEST_ASSERT_NULL(queue.fixed.p_items[0]);
    TEST_ASSERT_EQUAL_UINT32(0, queue.dynamic.count);
} /* test__ezq_push__capacity_full_buf__failure */

/*!
 * @brief Tests that \c ezq_push fails when the queue has a non-zero
 * capacity and the underlying fixed-size buffer and underlying
 * linked list combined have reached that capacity.
 */
static void
test__ezq_push__capacity_full_list__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Set any initial state. */
    queue.capacity = EZQ_FIXED_BUFFER_CAPACITY + 1;
    queue.fixed.count = EZQ_FIXED_BUFFER_CAPACITY;
    queue.dynamic.count = 1;
    queue.dynamic.p_head = NULL;
    queue.dynamic.p_tail = NULL;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(&queue, p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_FULL, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_UINT32(EZQ_FIXED_BUFFER_CAPACITY, queue.fixed.count);
    TEST_ASSERT_EQUAL_UINT32(1, queue.dynamic.count);
    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
} /* test__ezq_push__capacity_full_list__failure */

/*!
 * @brief Tests that \c ezq_push fails when it needs to push an item
 * onto the underlying linked list but has no dynamic memory allocation
 * function registered.
 */
static void
test__ezq_push__no_alloc_fn__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Set any initial state. */
    queue.alloc_fn = NULL;
    queue.fixed.count = EZQ_FIXED_BUFFER_CAPACITY;
    queue.dynamic.p_head = NULL;
    queue.dynamic.p_tail = NULL;
    queue.dynamic.count = 0;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(&queue, p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NO_ALLOC_FN, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_UINT32(EZQ_FIXED_BUFFER_CAPACITY, queue.fixed.count);
    TEST_ASSERT_EQUAL_UINT32(0, queue.dynamic.count);
    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
} /* test__ezq_push__no_alloc_fn__failure */

/*!
 * @brief Tests that \c ezq_push fails when it needs to push an item
 * onto the underlying linked list but its dynamic memory allocation
 * function fails.
 */
static void
test__ezq_push__alloc_fail__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Set any initial state. */
    queue.alloc_fn = custom_alloc_fn;
    queue.fixed.count = EZQ_FIXED_BUFFER_CAPACITY;
    queue.dynamic.p_head = NULL;
    queue.dynamic.p_tail = NULL;
    queue.dynamic.count = 0;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_push(&queue, p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_ALLOC_FAILURE, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_UINT32(EZQ_FIXED_BUFFER_CAPACITY, queue.fixed.count);
    TEST_ASSERT_EQUAL_UINT32(0, queue.dynamic.count);
    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
} /* test__ezq_push__alloc_fail__failure */

/*!
 * @brief Tests that \c ezq_pop success when the underlying fixed-size
 * buffer contains items but the underlying linked list does not.
 */
static void
test__ezq_pop__empty_list__success(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = NULL;

    /* Set any initial state. */
    queue.fixed.count = 1;
    queue.fixed.p_items[0] = (void *)0xFF;
    queue.dynamic.count = 0;
    queue.dynamic.p_head = NULL;
    queue.dynamic.p_tail = NULL;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_pop(&queue, (void **)&p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_SUCCESS, estat);
    TEST_ASSERT_EQUAL_PTR(0xFF, p_item);
    TEST_ASSERT_NULL(queue.fixed.p_items[0]);
    TEST_ASSERT_EQUAL_UINT32(0, queue.fixed.count);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL(0, queue.dynamic.count);
    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
} /* test__ezq_pop__empty_list__success */

/*!
 * @brief Tests that \c ezq_pop success when the underlying fixed-size
 * buffer contains items and the underlying linked list also contains
 * items.
 */
static void
test__ezq_pop__non_empty_list__success(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = NULL;
    struct ezq_linkedlist_node node = { 0 };

    /* Set any initial state. */
    node.p_item = (void *)0xFE;
    node.p_next = NULL;
    queue.free_fn = custom_free_fn;
    queue.fixed.count = EZQ_FIXED_BUFFER_CAPACITY;
    queue.fixed.p_items[0] = (void *)0xFF;
    queue.dynamic.count = 1;
    queue.dynamic.p_head = &node;
    queue.dynamic.p_tail = &node;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_pop(&queue, (void **)&p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_SUCCESS, estat);
    TEST_ASSERT_EQUAL_PTR(0xFF, p_item);
    TEST_ASSERT_EQUAL_PTR(0xFE, queue.fixed.p_items[0]);
    TEST_ASSERT_NULL(node.p_item);
    TEST_ASSERT_NULL(node.p_next);
    TEST_ASSERT_EQUAL_UINT32(EZQ_FIXED_BUFFER_CAPACITY, queue.fixed.count);
    TEST_ASSERT_EQUAL(0, queue.dynamic.count);
    TEST_ASSERT_NULL(queue.dynamic.p_head);
    TEST_ASSERT_NULL(queue.dynamic.p_tail);
} /* test__ezq_pop__non_empty_list__success */

/*!
 * @brief Tests that \c ezq_pop fails when passed an \c ezq_queue pointer
 * that is \c NULL .
 */
static void
test__ezq_pop__null_queue__failure(void)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_pop(NULL, (void **)&p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NULL_QUEUE, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_PTR(0xFF, p_item);
} /* test__ezq_pop__null_queue__failure */

/*!
 * @brief Tests that \c ezq_pop fails when passed an out variable
 * pointer that is \c NULL .
 */
static void
test__ezq_pop__null_out__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;

    /* Set any initial state. */
    queue.fixed.count = 1;
    queue.fixed.p_items[0] = (void *)0xFF;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_pop(&queue, NULL);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NULL_OUT, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_UINT32(1, queue.fixed.count);
    TEST_ASSERT_EQUAL_PTR(0xFF, queue.fixed.p_items[0]);
} /* test__ezq_pop__null_out__failure */

/*!
 * @brief Tests that \c ezq_pop fails when there are no items in the queue.
 */
static void
test__ezq_pop__empty__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;

    /* Set any initial state. */
    queue.fixed.count = 0;
    queue.dynamic.count = 0;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_pop(&queue, (void **)&p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_EMPTY, estat);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_PTR(0xFF, p_item);
} /* test__ezq_pop__empty__failure */

/*!
 * @brief Tests that \c ezq_pop fails when the underlying linked list
 * contains items but the queue has no freeing function registered.
 */
static void
test__ezq_pop__no_free_fn__failure(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    int *p_item = (int *)0xFF;
    struct ezq_linkedlist_node node = { 0 };

    /* Set any initial state. */
    node.p_item = (void *)0xFE;
    node.p_next = NULL;
    queue.free_fn = NULL;
    queue.fixed.count = EZQ_FIXED_BUFFER_CAPACITY;
    queue.dynamic.count = 1;
    queue.dynamic.p_head = &node;
    queue.dynamic.p_tail = &node;

    /* Invoke the function being tested and verify the expected outcome. */
    estat = ezq_pop(&queue, (void **)&p_item);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NO_FREE_FN, estat);
    TEST_ASSERT_EQUAL_UINT32(EZQ_FIXED_BUFFER_CAPACITY, queue.fixed.count);
    TEST_ASSERT_EQUAL_UINT32(1, queue.dynamic.count);
    TEST_ASSERT_EQUAL_PTR(&node, queue.dynamic.p_head);
    TEST_ASSERT_EQUAL_PTR(&node, queue.dynamic.p_tail);
    TEST_ASSERT_EQUAL_PTR(0xFE, node.p_item);
    TEST_ASSERT_NULL(node.p_next);

    /* Validate that nothing else was unexpectedly modified. */
    TEST_ASSERT_EQUAL_PTR(0xFF, p_item);
} /* test__ezq_pop__no_free_fn__failure */

/*!
 * @brief Tests that \c ezq_count succeeds properly when the queue contains
 * no items.
 */
static void
test__ezq_count__zero_count__success(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    unsigned int count = 0;

    /* Set any initial state. */
    queue.fixed.count = 0;
    queue.dynamic.count = 0;

    /* Invoke the function being tested and verify the expected outcome. */
    count = ezq_count(&queue, &estat);
    TEST_ASSERT_EQUAL_UINT(0, count);
    TEST_ASSERT_EQUAL(EZQ_STATUS_SUCCESS, estat);
} /* test__ezq_count__zero_count__success */

/*!
 * @brief Tests that \c ezq_count succeeds properly when the queue contains
 * any items.
 */
static void
test__ezq_count__non_zero_count__success(void)
{
    ezq_queue queue = { 0 };
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    unsigned int count = 0;

    /* Set any initial state. */
    queue.fixed.count = 5;
    queue.dynamic.count = 3;

    /* Invoke the function being tested and verify the expected outcome. */
    count = ezq_count(&queue, &estat);
    TEST_ASSERT_EQUAL_UINT(queue.fixed.count + queue.dynamic.count, count);
    TEST_ASSERT_EQUAL(EZQ_STATUS_SUCCESS, estat);
} /* test__ezq_count__non_zero_count__success */

/*!
 * @brief Test that \c ezq_count fails when passed an \c ezq_queue pointer
 * that is \c NULL .
 */
static void
test__ezq_count__null_queue__failure(void)
{
    ezq_status estat = EZQ_STATUS_UNKNOWN;
    unsigned int count = 0;

    /* Invoke the function being tested and verify the expected outcome. */
    count = ezq_count(NULL, &estat);
    TEST_ASSERT_EQUAL_UINT(0, count);
    TEST_ASSERT_EQUAL(EZQ_STATUS_NULL_QUEUE, estat);
} /* test__ezq_count__null_queue__failure */

static void
test__ezq_destroy__empty_queue__success(void)
{

} /* test__ezq_destroy__empty_queue__success */

static void
test__ezq_destroy__non_null_cleanup_fn__success(void)
{

} /* test__ezq_destroy__non_null_cleanup_fn__success */

static void
test__ezq_destroy__null_cleanup_fn__success(void)
{

} /* test__ezq_destroy__null_cleanup_fn__success */

static void
test__ezq_destroy__null_queue__failure(void)
{

} /* test__ezq_destroy__null_queue__failure */

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test__ezq_init__standard__success);
    RUN_TEST(test__ezq_init__null_queue__failure);

    RUN_TEST(test__ezq_push__buf__success);
    RUN_TEST(test__ezq_push__list__success);
    RUN_TEST(test__ezq_push__null_queue__failure);
    RUN_TEST(test__ezq_push__null_item__failure);
    RUN_TEST(test__ezq_push__capacity_full_buf__failure);
    RUN_TEST(test__ezq_push__capacity_full_list__failure);
    RUN_TEST(test__ezq_push__no_alloc_fn__failure);
    RUN_TEST(test__ezq_push__alloc_fail__failure);

    RUN_TEST(test__ezq_pop__empty_list__success);
    RUN_TEST(test__ezq_pop__non_empty_list__success);
    RUN_TEST(test__ezq_pop__null_queue__failure);
    RUN_TEST(test__ezq_pop__null_out__failure);
    RUN_TEST(test__ezq_pop__empty__failure);
    RUN_TEST(test__ezq_pop__no_free_fn__failure);

    RUN_TEST(test__ezq_count__zero_count__success);
    RUN_TEST(test__ezq_count__non_zero_count__success);
    RUN_TEST(test__ezq_count__null_queue__failure);

    RUN_TEST(test__ezq_destroy__empty_queue__success);
    RUN_TEST(test__ezq_destroy__null_cleanup_fn__success);
    RUN_TEST(test__ezq_destroy__non_null_cleanup_fn__success);
    RUN_TEST(test__ezq_destroy__null_queue__failure);

    return UNITY_END();
} /* main */
