#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stddef.h>
#include <stdint.h>

#define MP_INVALID 0xFFFFu

typedef struct
{
    uint8_t  *storage;   /* block_count * block_size bytes */
    uint16_t *next_free; /* Free list: next_free[i] = next free block index */
    uint8_t  *in_use;    /* in_use[i] = 1 if allocated (for double-free detection) */
    size_t    block_size;
    uint16_t  block_count;
    uint16_t  free_head; /* Index of first free block, or MP_INVALID */
    uint16_t  free_blocks;
} mem_pool_t;

typedef enum
{
    MP_OK = 0,
    MP_ERR_NULL,
    MP_ERR_EXHAUSTED,
    MP_ERR_BAD_POINTER,
    MP_ERR_DOUBLE_FREE,
    MP_ERR_RANGE,
    MP_ERR_CORRUPT
} mp_status_t;

mp_status_t mp_init(mem_pool_t *mp,
                    uint8_t    *storage,
                    uint16_t   *next_free,
                    uint8_t    *in_use,
                    size_t      block_size,
                    uint16_t    block_count);
mp_status_t mp_alloc(mem_pool_t *mp, void **out);
mp_status_t mp_free(mem_pool_t *mp, void *block);
uint16_t    mp_free_count(const mem_pool_t *mp);

#endif