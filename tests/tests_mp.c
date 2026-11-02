#include <stdio.h>
#include <stdlib.h>

#include "mem_pool.h"

#define TEST_BLOCK_SIZE   4
#define TEST_BLOCK_COUNT  3
#define TEST_STORAGE_SIZE (TEST_BLOCK_SIZE * TEST_BLOCK_COUNT)

typedef struct
{
    mem_pool_t pool;
    uint8_t    storage[TEST_STORAGE_SIZE];
    uint16_t   next_free[TEST_BLOCK_COUNT];
    uint8_t    in_use[TEST_BLOCK_COUNT];
} mp_fixture_t;

static const char *mp_status_name(mp_status_t status)
{
    switch (status)
    {
        case MP_OK:
            return "MP_OK";
        case MP_ERR_NULL:
            return "MP_ERR_NULL";
        case MP_ERR_EXHAUSTED:
            return "MP_ERR_EXHAUSTED";
        case MP_ERR_BAD_POINTER:
            return "MP_ERR_BAD_POINTER";
        case MP_ERR_DOUBLE_FREE:
            return "MP_ERR_DOUBLE_FREE";
        case MP_ERR_RANGE:
            return "MP_ERR_RANGE";
        case MP_ERR_CORRUPT:
            return "MP_ERR_CORRUPT";
        default:
            return "UNKNOWN_MP_STATUS";
    }
}

static int check_status_at(mp_status_t received, mp_status_t expected, const char *file, int line)
{
    if (received == expected)
    {
        return 1;
    }

    fprintf(stderr,
            "Test failed, File: %s, Line: %d, Expected: %s, Received: %s.\n",
            file,
            line,
            mp_status_name(expected),
            mp_status_name(received));
    return 0;
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_status(received, expected) check_status_at((received), (expected), __FILE__, __LINE__)

static int check_size_at(size_t received, size_t expected, const char *file, int line)
{
    if (received == expected)
    {
        return 1;
    }

    fprintf(stderr, "Test failed, File: %s, Line: %d, Expected: %zu, Received: %zu.\n", file, line, expected, received);
    return 0;
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_size(received, expected) check_size_at((received), (expected), __FILE__, __LINE__)

static int check_u16_at(uint16_t received, uint16_t expected, const char *file, int line)
{
    if (received == expected)
    {
        return 1;
    }

    fprintf(stderr,
            "Test failed, File: %s, Line: %d, Expected: %u, Received: %u.\n",
            file,
            line,
            (unsigned int)expected,
            (unsigned int)received);
    return 0;
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_u16(received, expected) check_u16_at((received), (expected), __FILE__, __LINE__)

static int check_byte_at(uint8_t received, uint8_t expected, const char *file, int line)
{
    if (received == expected)
    {
        return 1;
    }

    fprintf(stderr,
            "Test failed, File: %s, Line: %d, Expected: %u, Received: %u.\n",
            file,
            line,
            (unsigned int)expected,
            (unsigned int)received);
    return 0;
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_byte(received, expected) check_byte_at((received), (expected), __FILE__, __LINE__)

static int check_bool_at(int received, int expected, const char *file, int line)
{
    if (received == expected)
    {
        return 1;
    }

    fprintf(stderr, "Test failed, File: %s, Line: %d, Expected: %d, Received: %d.\n", file, line, expected, received);
    return 0;
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check_bool(received, expected) check_bool_at((received), (expected), __FILE__, __LINE__)

static mp_status_t setup_mp(mp_fixture_t *fixture)
{
    return mp_init(&fixture->pool,
                   fixture->storage,
                   fixture->next_free,
                   fixture->in_use,
                   TEST_BLOCK_SIZE,
                   TEST_BLOCK_COUNT);
}

int main(void)
{
    int all_passed = 1;

    /* ---------- mp_init tests ---------- */

    uint8_t    storage_init   = 0;
    uint16_t   next_free_init = 0;
    uint8_t    in_use_init    = 0;
    mem_pool_t pool_init_test = { 0 };

    all_passed &= check_status(mp_init(NULL, &storage_init, &next_free_init, &in_use_init, 1, 1), MP_ERR_NULL);
    all_passed &= check_status(mp_init(&pool_init_test, NULL, &next_free_init, &in_use_init, 1, 1), MP_ERR_NULL);
    all_passed &= check_status(mp_init(&pool_init_test, &storage_init, NULL, &in_use_init, 1, 1), MP_ERR_NULL);
    all_passed &= check_status(mp_init(&pool_init_test, &storage_init, &next_free_init, NULL, 1, 1), MP_ERR_NULL);
    all_passed &=
        check_status(mp_init(&pool_init_test, &storage_init, &next_free_init, &in_use_init, 0, 1), MP_ERR_RANGE);
    all_passed &=
        check_status(mp_init(&pool_init_test, &storage_init, &next_free_init, &in_use_init, 1, 0), MP_ERR_RANGE);
    all_passed &=
        check_status(mp_init(&pool_init_test, &storage_init, &next_free_init, &in_use_init, SIZE_MAX / 2 + 1, 2),
                     MP_ERR_RANGE);

    mp_fixture_t init_fixture  = { 0 };
    all_passed                &= check_status(setup_mp(&init_fixture), MP_OK);
    all_passed                &= check_size(init_fixture.pool.block_size, TEST_BLOCK_SIZE);
    all_passed                &= check_u16(init_fixture.pool.block_count, TEST_BLOCK_COUNT);
    all_passed                &= check_u16(init_fixture.pool.free_head, 0);
    all_passed                &= check_u16(init_fixture.pool.free_blocks, TEST_BLOCK_COUNT);
    all_passed                &= check_u16(init_fixture.next_free[0], 1);
    all_passed                &= check_u16(init_fixture.next_free[1], 2);
    all_passed                &= check_u16(init_fixture.next_free[2], MP_INVALID);
    all_passed                &= check_byte(init_fixture.in_use[0], 0);
    all_passed                &= check_byte(init_fixture.in_use[1], 0);
    all_passed                &= check_byte(init_fixture.in_use[2], 0);

    /* ---------- mp_alloc tests ---------- */

    void *out_null  = (void *)init_fixture.storage;
    all_passed     &= check_status(mp_alloc(NULL, &out_null), MP_ERR_NULL);
    all_passed     &= check_status(mp_alloc(&init_fixture.pool, NULL), MP_ERR_NULL);

    mp_fixture_t alloc_fixture  = { 0 };
    all_passed                 &= check_status(setup_mp(&alloc_fixture), MP_OK);

    void *allocated[TEST_BLOCK_COUNT] = { NULL };
    for (uint16_t i = 0; i < TEST_BLOCK_COUNT; i++)
    {
        all_passed &= check_status(mp_alloc(&alloc_fixture.pool, &allocated[i]), MP_OK);
        all_passed &= check_bool(allocated[i] == alloc_fixture.storage + (size_t)i * TEST_BLOCK_SIZE, 1);
        all_passed &= check_byte(alloc_fixture.in_use[i], 1);
        all_passed &= check_u16(alloc_fixture.pool.free_blocks, TEST_BLOCK_COUNT - i - 1);
    }

    void *out_exhausted  = (void *)alloc_fixture.storage;
    all_passed          &= check_status(mp_alloc(&alloc_fixture.pool, &out_exhausted), MP_ERR_EXHAUSTED);
    all_passed          &= check_bool(out_exhausted == NULL, 1);
    all_passed          &= check_u16(alloc_fixture.pool.free_head, MP_INVALID);
    all_passed          &= check_u16(mp_free_count(&alloc_fixture.pool), 0);
    all_passed          &= check_u16(mp_free_count(NULL), 0);

    /* Allocation rejects corrupted free-list metadata without changing it. */
    mp_fixture_t alloc_corrupt_head    = { 0 };
    all_passed                        &= check_status(setup_mp(&alloc_corrupt_head), MP_OK);
    alloc_corrupt_head.pool.free_head  = TEST_BLOCK_COUNT;
    void *out_corrupt                  = (void *)alloc_corrupt_head.storage;
    all_passed                        &= check_status(mp_alloc(&alloc_corrupt_head.pool, &out_corrupt), MP_ERR_CORRUPT);
    all_passed                        &= check_bool(out_corrupt == NULL, 1);

    mp_fixture_t alloc_corrupt_in_use  = { 0 };
    all_passed                        &= check_status(setup_mp(&alloc_corrupt_in_use), MP_OK);
    alloc_corrupt_in_use.in_use[0]     = 1;
    all_passed &= check_status(mp_alloc(&alloc_corrupt_in_use.pool, &out_corrupt), MP_ERR_CORRUPT);
    all_passed &= check_u16(alloc_corrupt_in_use.pool.free_head, 0);
    all_passed &= check_u16(alloc_corrupt_in_use.pool.free_blocks, TEST_BLOCK_COUNT);

    mp_fixture_t alloc_corrupt_next  = { 0 };
    all_passed                      &= check_status(setup_mp(&alloc_corrupt_next), MP_OK);
    alloc_corrupt_next.next_free[0]  = TEST_BLOCK_COUNT;
    all_passed                      &= check_status(mp_alloc(&alloc_corrupt_next.pool, &out_corrupt), MP_ERR_CORRUPT);
    all_passed                      &= check_u16(alloc_corrupt_next.pool.free_head, 0);
    all_passed                      &= check_byte(alloc_corrupt_next.in_use[0], 0);
    all_passed                      &= check_u16(alloc_corrupt_next.pool.free_blocks, TEST_BLOCK_COUNT);

    /* ---------- mp_free tests ---------- */

    all_passed &= check_status(mp_free(NULL, allocated[0]), MP_ERR_NULL);
    all_passed &= check_status(mp_free(&alloc_fixture.pool, NULL), MP_ERR_NULL);

    mp_fixture_t free_fixture  = { 0 };
    all_passed                &= check_status(setup_mp(&free_fixture), MP_OK);
    void *first                = NULL;
    all_passed                &= check_status(mp_alloc(&free_fixture.pool, &first), MP_OK);
    all_passed                &= check_status(mp_free(&free_fixture.pool, first), MP_OK);
    all_passed                &= check_byte(free_fixture.in_use[0], 0);
    all_passed                &= check_u16(free_fixture.next_free[0], 1);
    all_passed                &= check_u16(free_fixture.pool.free_head, 0);
    all_passed                &= check_u16(mp_free_count(&free_fixture.pool), TEST_BLOCK_COUNT);

    void *reallocated  = NULL;
    all_passed        &= check_status(mp_alloc(&free_fixture.pool, &reallocated), MP_OK);
    all_passed        &= check_bool(reallocated == first, 1);

    /* Freeing the same block twice reports a double free. */
    all_passed                        &= check_status(mp_free(&free_fixture.pool, first), MP_OK);
    uint16_t head_before_double_free   = free_fixture.pool.free_head;
    uint16_t count_before_double_free  = free_fixture.pool.free_blocks;
    all_passed                        &= check_status(mp_free(&free_fixture.pool, first), MP_ERR_DOUBLE_FREE);
    all_passed                        &= check_u16(free_fixture.pool.free_head, head_before_double_free);
    all_passed                        &= check_u16(free_fixture.pool.free_blocks, count_before_double_free);

    /* Reject interior, one-past-end, and unrelated addresses. */
    all_passed &= check_status(mp_free(&free_fixture.pool, free_fixture.storage + 1), MP_ERR_BAD_POINTER);
    all_passed &= check_status(mp_free(&free_fixture.pool, free_fixture.storage + sizeof free_fixture.storage),
                               MP_ERR_BAD_POINTER);
    uint8_t foreign_storage[TEST_STORAGE_SIZE] = { 0 };
    all_passed &= check_status(mp_free(&free_fixture.pool, foreign_storage), MP_ERR_BAD_POINTER);

    /* Pointer errors do not modify pool bookkeeping. */
    all_passed &= check_u16(free_fixture.pool.free_head, head_before_double_free);
    all_passed &= check_u16(free_fixture.pool.free_blocks, count_before_double_free);

    /* A valid block address with a corrupt in-use flag is detected. */
    mp_fixture_t free_corrupt_in_use  = { 0 };
    all_passed                       &= check_status(setup_mp(&free_corrupt_in_use), MP_OK);
    void *corrupt_block               = NULL;
    all_passed                       &= check_status(mp_alloc(&free_corrupt_in_use.pool, &corrupt_block), MP_OK);
    free_corrupt_in_use.in_use[0]     = 2;
    all_passed                       &= check_status(mp_free(&free_corrupt_in_use.pool, corrupt_block), MP_ERR_CORRUPT);
    all_passed                       &= check_u16(free_corrupt_in_use.pool.free_head, 1);
    all_passed                       &= check_u16(free_corrupt_in_use.pool.free_blocks, TEST_BLOCK_COUNT - 1);

    /* Corrupt free-list state is rejected before the free operation mutates the pool. */
    mp_fixture_t free_corrupt_head    = { 0 };
    all_passed                       &= check_status(setup_mp(&free_corrupt_head), MP_OK);
    void *head_test_block             = NULL;
    all_passed                       &= check_status(mp_alloc(&free_corrupt_head.pool, &head_test_block), MP_OK);
    free_corrupt_head.pool.free_head  = TEST_BLOCK_COUNT;
    uint8_t  in_use_before_bad_head   = free_corrupt_head.in_use[0];
    uint16_t count_before_bad_head    = free_corrupt_head.pool.free_blocks;
    all_passed                       &= check_status(mp_free(&free_corrupt_head.pool, head_test_block), MP_ERR_CORRUPT);
    all_passed                       &= check_byte(free_corrupt_head.in_use[0], in_use_before_bad_head);
    all_passed                       &= check_u16(free_corrupt_head.pool.free_blocks, count_before_bad_head);

    mp_fixture_t free_corrupt_count      = { 0 };
    all_passed                          &= check_status(setup_mp(&free_corrupt_count), MP_OK);
    void *count_test_block               = NULL;
    all_passed                          &= check_status(mp_alloc(&free_corrupt_count.pool, &count_test_block), MP_OK);
    free_corrupt_count.pool.free_blocks  = free_corrupt_count.pool.block_count;
    all_passed &= check_status(mp_free(&free_corrupt_count.pool, count_test_block), MP_ERR_CORRUPT);
    all_passed &= check_byte(free_corrupt_count.in_use[0], 1);
    all_passed &= check_u16(free_corrupt_count.pool.free_head, 1);

    /* Returns whether all tests passed. */
    if (all_passed)
    {
        printf("All tests passed correctly.\n");
    }
    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}