#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ring_buffer.h"

/* Switch for translating error numbers into names */
static const char *rb_status_name(rb_status_t status)
{
    switch (status)
    {
        case RB_OK:
            return "RB_OK";
        case RB_ERR_NULL:
            return "RB_ERR_NULL";
        case RB_ERR_FULL:
            return "RB_ERR_FULL";
        case RB_ERR_EMPTY:
            return "RB_ERR_EMPTY";
        case RB_ERR_RANGE:
            return "RB_ERR_RANGE";
        default:
            return "UNKNOWN_RB_STATUS";
    }
}

/* Switch for translating bool values into names */
static const char *rb_bool_name(bool boolean)
{
    switch (boolean)
    {
        case 0:
            return "TRUE";
        case 1:
            return "FALSE";
        default:
            return "UNKNOWN_BOOLEAN";
    }
}

/* Helper function for comparing two statuses */
static int check_status_at(rb_status_t received, rb_status_t expected, const char *file, int line)
{
    if (received == expected)
    { /* If equal return nothing */
        return 1;
    }
    else
    { /* If not equal return file, line and expected and received values */
        fprintf(stderr,
                "Test failed, File: %s, Line: %d, Expected: %s, Received: %s.\n",
                file,
                line,
                rb_status_name(expected),
                rb_status_name(received));
        return 0;
    }
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_status(received, expected) check_status_at((received), (expected), __FILE__, __LINE__)

/* Helper function for comparing two bytes */
static int check_byte_at(uint8_t received, uint8_t expected, const char *file, int line)
{
    if (received == expected)
    { /* If equal return nothing */
        return 1;
    }
    else
    { /* If not equal return file, line and expected and received values */
        fprintf(stderr,
                "Test failed, File: %s, Line: %d, Expected: %d, Received: %d.\n",
                file,
                line,
                expected,
                received);
        return 0;
    }
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_byte(received, expected) check_byte_at((received), (expected), __FILE__, __LINE__)

/* Helper function for comparing two size_t */
static int check_size_at(size_t received, size_t expected, const char *file, int line)
{
    if (received == expected)
    { /* If equal return nothing */
        return 1;
    }
    else
    { /* If not equal return file, line and expected and received values */
        fprintf(stderr,
                "Test failed, File: %s, Line: %d, Expected: %zu, Received: %zu.\n",
                file,
                line,
                expected,
                received);
        return 0;
    }
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_size(received, expected) check_size_at((received), (expected), __FILE__, __LINE__)

/* Helper function for comparing two size_t */
static int check_bool_at(bool received, bool expected, const char *file, int line)
{
    if (received == expected)
    { /* If equal return nothing */
        return 1;
    }
    else
    { /* If not equal return file, line and expected and received values */
        fprintf(stderr,
                "Test failed, File: %s, Line: %d, Expected: %d, Received: %d.\n",
                file,
                line,
                rb_bool_name(expected),
                rb_bool_name(received));
        return 0;
    }
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_bool(received, expected) check_bool_at((received), (expected), __FILE__, __LINE__)

int main(void)
{
    int all_passed = 1;

    /* ----------rb_init tests---------- */

    /* Null arguments */
    ring_buffer_t rb_null_1       = { 0 };
    uint8_t       storage_null_1  = 0;
    size_t        capacity_null_1 = 1;

    all_passed &= check_status(rb_init(NULL, &storage_null_1, capacity_null_1), RB_ERR_NULL);
    all_passed &= check_status(rb_init(&rb_null_1, NULL, capacity_null_1), RB_ERR_NULL);

    /* Capacity equals cero */
    ring_buffer_t rb_torage_range = { 0 };
    uint8_t       storage_range   = 0;

    all_passed &= check_status(rb_init(&rb_torage_range, &storage_range, 0), RB_ERR_RANGE);

    /* ----------rb_push tests---------- */

    /* Null arguments */
    ring_buffer_t rb_null_2   = { 0 };
    uint8_t       byte_null_2 = 0;

    all_passed &= check_status(rb_push(NULL, byte_null_2), RB_ERR_NULL);
    all_passed &= check_status(rb_push(&rb_null_2, byte_null_2), RB_ERR_NULL);

    /* Full buffer */
    ring_buffer_t rb_full      = { 0 };
    uint8_t       storage_full = 0;

    all_passed &= check_status(rb_init(&rb_full, &storage_full, 1), RB_OK);
    all_passed &= check_status(rb_push(&rb_full, 0), RB_OK);
    all_passed &= check_status(rb_push(&rb_full, 1), RB_ERR_FULL);

    /* ----------rb_pop tests---------- */

    /* Null arguments */
    uint8_t popped  = 0;
    all_passed     &= check_status(rb_pop(NULL, &popped), RB_ERR_NULL);

    ring_buffer_t rb_pop_test    = { 0 };
    uint8_t       pop_storage[3] = { 0 };

    all_passed &= check_status(rb_init(&rb_pop_test, pop_storage, 3), RB_OK);

    /* Empty buffer */
    all_passed &= check_status(rb_pop(&rb_pop_test, &popped), RB_ERR_EMPTY);

    /* FIFO order and wraparound */
    all_passed &= check_status(rb_push(&rb_pop_test, 10), RB_OK);
    all_passed &= check_status(rb_push(&rb_pop_test, 20), RB_OK);
    all_passed &= check_status(rb_push(&rb_pop_test, 30), RB_OK);

    all_passed &= check_status(rb_pop(&rb_pop_test, &popped), RB_OK);
    all_passed &= check_byte(popped, 10);

    all_passed &= check_status(rb_pop(&rb_pop_test, &popped), RB_OK);
    all_passed &= check_byte(popped, 20);

    /* Push two bytes; the write index wraps to the start of the storage here. */
    all_passed &= check_status(rb_push(&rb_pop_test, 40), RB_OK);
    all_passed &= check_status(rb_push(&rb_pop_test, 50), RB_OK);

    all_passed &= check_status(rb_pop(&rb_pop_test, &popped), RB_OK);
    all_passed &= check_byte(popped, 30);

    all_passed &= check_status(rb_pop(&rb_pop_test, &popped), RB_OK);
    all_passed &= check_byte(popped, 40);

    all_passed &= check_status(rb_pop(&rb_pop_test, &popped), RB_OK);
    all_passed &= check_byte(popped, 50);

    all_passed &= check_status(rb_pop(&rb_pop_test, &popped), RB_ERR_EMPTY);
    all_passed &= check_status(rb_pop(&rb_pop_test, NULL), RB_ERR_NULL);

    /* ----------rb_peek tests---------- */

    /* Null arguments */
    ring_buffer_t rb_null_3     = { 0 };
    size_t        offset_null_3 = 1;
    uint8_t       out_null_3    = 0;
    size_t        len_null_3    = 5;

    all_passed &= check_status(rb_peek(NULL, offset_null_3, &out_null_3, len_null_3), RB_ERR_NULL);
    all_passed &= check_status(rb_peek(&rb_null_3, offset_null_3, NULL, len_null_3), RB_ERR_NULL);

    /* Empty ring buffer */
    ring_buffer_t rb_empty = { 0 };
    uint8_t       empty_storage[3];
    size_t        offset_empty = 1;
    uint8_t       out_empty    = 0;
    size_t        len_empty    = 5;

    all_passed &= check_status(rb_init(&rb_empty, empty_storage, sizeof empty_storage), RB_OK);
    all_passed &= check_status(rb_peek(&rb_empty, offset_empty, &out_empty, len_empty), RB_ERR_RANGE);

    /* Rejects a range extending past the stored data */
    ring_buffer_t rb_peek_range = { 0 };
    uint8_t       peek_storage[3];
    uint8_t       peek_out[2] = { 0xAA, 0xBB };

    all_passed &= check_status(rb_init(&rb_peek_range, peek_storage, sizeof peek_storage), RB_OK);
    all_passed &= check_status(rb_push(&rb_peek_range, 10), RB_OK);
    all_passed &= check_status(rb_push(&rb_peek_range, 20), RB_OK);

    size_t peek_count_before  = rb_count(&rb_peek_range);
    all_passed               &= check_status(rb_peek(&rb_peek_range, 1, peek_out, 2), RB_ERR_RANGE);

    all_passed &= check_size(rb_count(&rb_peek_range), peek_count_before);
    all_passed &= check_byte(peek_out[0], 0xAA);
    all_passed &= check_byte(peek_out[1], 0xBB);

    /* Peek across the storage wrap boundary without consuming bytes */
    ring_buffer_t rb_peek_wrap = { 0 };
    uint8_t       peek_wrap_storage[4];
    uint8_t       peek_wrap_out[3]      = { 0 };
    const uint8_t expected_peek_wrap[3] = { 4, 5, 6 };

    all_passed &= check_status(rb_init(&rb_peek_wrap, peek_wrap_storage, sizeof peek_wrap_storage), RB_OK);

    /* Fill the buffer, then pop two bytes to move tail forward. */
    all_passed &= check_status(rb_push(&rb_peek_wrap, 1), RB_OK);
    all_passed &= check_status(rb_push(&rb_peek_wrap, 2), RB_OK);
    all_passed &= check_status(rb_push(&rb_peek_wrap, 3), RB_OK);
    all_passed &= check_status(rb_push(&rb_peek_wrap, 4), RB_OK);
    all_passed &= check_status(rb_pop(&rb_peek_wrap, &popped), RB_OK);
    all_passed &= check_status(rb_pop(&rb_peek_wrap, &popped), RB_OK);

    /* These pushes wrap around to the start of storage. */
    all_passed &= check_status(rb_push(&rb_peek_wrap, 5), RB_OK);
    all_passed &= check_status(rb_push(&rb_peek_wrap, 6), RB_OK);

    size_t count_before_peek  = rb_count(&rb_peek_wrap);
    all_passed               &= check_status(rb_peek(&rb_peek_wrap, 1, peek_wrap_out, sizeof peek_wrap_out), RB_OK);

    all_passed &= check_bool(memcmp(peek_wrap_out, expected_peek_wrap, sizeof expected_peek_wrap), 0);

    all_passed &= check_size(rb_count(&rb_peek_wrap), count_before_peek);

    /* ----------rb_discard tests---------- */

    /* Null arguments */
    size_t n_null = 0;

    all_passed &= check_status(rb_discard(NULL, n_null), RB_ERR_NULL);

    /* Number of desired deleted bytes exceeds ring buffer's currently stored bytes */
    ring_buffer_t rb_range = { 0 };
    uint8_t       range_discard[1];

    all_passed &= check_status(rb_init(&rb_range, range_discard, sizeof range_discard), RB_OK);

    size_t n_range = 1;

    all_passed &= check_status(rb_discard(&rb_range, n_range), RB_ERR_RANGE);

    /* An out-of-range discard leaves count and contents unchanged. */
    ring_buffer_t rb_discard_range = { 0 };
    uint8_t       discard_storage[3];
    uint8_t       contents_before[3];
    uint8_t       contents_after[3];
    const uint8_t expected_contents[3] = { 10, 20, 30 };

    all_passed &= check_status(rb_init(&rb_discard_range, discard_storage, sizeof discard_storage), RB_OK);
    all_passed &= check_status(rb_push(&rb_discard_range, 10), RB_OK);
    all_passed &= check_status(rb_push(&rb_discard_range, 20), RB_OK);
    all_passed &= check_status(rb_push(&rb_discard_range, 30), RB_OK);

    size_t count_before  = rb_count(&rb_discard_range);
    all_passed          &= check_status(rb_peek(&rb_discard_range, 0, contents_before, count_before), RB_OK);

    all_passed &= check_status(rb_discard(&rb_discard_range, count_before + 1), RB_ERR_RANGE);

    all_passed &= check_size(rb_count(&rb_discard_range), count_before);

    all_passed &= check_status(rb_peek(&rb_discard_range, 0, contents_after, count_before), RB_OK);

    all_passed &= check_bool(memcmp(contents_before, contents_after, count_before), 0);
    all_passed &= check_bool(memcmp(contents_after, expected_contents, count_before), 0);

    /* Working rb_discard funciton */
    ring_buffer_t rb_discard_test         = { 0 };
    uint8_t       discard_test_storage[3] = { 0 };

    all_passed &= check_status(rb_init(&rb_discard_test, discard_test_storage, 3), RB_OK);
    all_passed &= check_status(rb_push(&rb_discard_test, 10), RB_OK);
    all_passed &= check_status(rb_discard(&rb_discard_test, 1), RB_OK);

    /* ---------- rb_count tests ---------- */

    /* Null buffer: rb_count is defined to return 0 for NULL. */
    all_passed &= check_byte(rb_count(NULL), 0);

    /* Count changes as bytes are pushed and popped. */
    ring_buffer_t rb_count_test = { 0 };
    uint8_t       count_storage[2];
    uint8_t       popped_count;

    all_passed &= check_status(rb_init(&rb_count_test, count_storage, sizeof count_storage), RB_OK);

    all_passed &= check_byte(rb_count(&rb_count_test), 0);

    all_passed &= check_status(rb_push(&rb_count_test, 10), RB_OK);
    all_passed &= check_byte(rb_count(&rb_count_test), 1);

    all_passed &= check_status(rb_push(&rb_count_test, 20), RB_OK);
    all_passed &= check_byte(rb_count(&rb_count_test), 2);

    all_passed &= check_status(rb_pop(&rb_count_test, &popped_count), RB_OK);
    all_passed &= check_byte(rb_count(&rb_count_test), 1);

    /* ----------rb_free tests---------- */

    /* Null buffer */
    all_passed &= check_byte(rb_free(NULL), 0);

    /* Empty, partially full, then full */
    ring_buffer_t rb_free_test = { 0 };
    uint8_t       free_storage[2];

    all_passed &= check_status(rb_init(&rb_free_test, free_storage, sizeof free_storage), RB_OK);

    all_passed &= check_byte(rb_free(&rb_free_test), 2);

    all_passed &= check_status(rb_push(&rb_free_test, 10), RB_OK);

    all_passed &= check_byte(rb_free(&rb_free_test), 1);

    all_passed &= check_status(rb_push(&rb_free_test, 20), RB_OK);

    all_passed &= check_byte(rb_free(&rb_free_test), 0);

    /* Returns wether all the tests have passed or not */
    if (all_passed == 1)
    {
        printf("All tests passed correctly.\n");
    }
    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}