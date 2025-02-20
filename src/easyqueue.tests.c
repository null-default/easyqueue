#include <stdio.h>
#include <unity/unity.h>
#include "easyqueue.h"

void setUp(void) { } /* UNUSED; required definition for Unity tests */
void tearDown(void) { } /* UNUSED; required definition for Unity tests */

/*!
 * @brief Dummy function that returns a pointer value that is not \c NULL .
 *
 * @param[in] size UNUSED
 * @return In all cases, a pointer value that is not \c NULL .
 */
static void *
alloc_fn_success(const size_t size)
{
    (void)size;
    return (void *)(-1);
} /* alloc_fn_success */

/*!
 * @brief Dummy function that returns a \c NULL pointer value.
 *
 * @param[in] size UNUSED
 * @return In all cases, \c NULL .
 */
static void *
alloc_fn_failure(const size_t size)
{
    (void)size;
    return NULL;
} /* alloc_fn_failure */

/*!
 * @brief Dummy function that matches the signature of \c free() , but
 * does not actually perform any operations.
 *
 * @param[in] ptr UNUSED
 */
static void
free_fn_no_op(void * const ptr)
{
    (void)ptr;
} /* free_fn_no_op */

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

    estat = ezq_init(&queue, TEST_CAPACITY, alloc_fn_success, free_fn_no_op);
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
    TEST_ASSERT_EQUAL_PTR(queue.alloc_fn, alloc_fn_success);
    TEST_ASSERT_EQUAL_PTR(queue.free_fn, free_fn_no_op);
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

    estat = ezq_init(NULL, TEST_CAPACITY, alloc_fn_success, free_fn_no_op);
    TEST_ASSERT_EQUAL_UINT8(EZQ_STATUS_NULL_QUEUE, estat);
} /* test__ezq_init__null_queue__failure */

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test__ezq_init__standard__success);
    RUN_TEST(test__ezq_init__null_queue__failure);

    return UNITY_END();
} /* main */
