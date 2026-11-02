#include <stdint.h>

#include "mem_pool.h"

/* Initializes a memory pool with the given parameters if possible */
mp_status_t
mp_init(mem_pool_t *mp, uint8_t *storage, uint16_t *next_free, uint8_t *in_use, size_t block_size, uint16_t block_count)
{
    if (mp == NULL || storage == NULL || next_free == NULL || in_use == NULL)
    {
        return MP_ERR_NULL;
    }
    if (block_size == 0 || block_count == 0)
    {
        return MP_ERR_RANGE;
    }
    if (block_count > SIZE_MAX / block_size)
    {
        return MP_ERR_RANGE;
    }

    mp->storage     = storage;
    mp->next_free   = next_free;
    mp->in_use      = in_use;
    mp->block_size  = block_size;
    mp->block_count = block_count;

    for (uint16_t i = 0; i < block_count; i++)
    {
        mp->next_free[i] = (uint16_t)(i + 1);
        mp->in_use[i]    = 0;
    }

    mp->next_free[mp->block_count - 1] = MP_INVALID;
    mp->free_head                      = 0;
    mp->free_blocks                    = mp->block_count;

    return MP_OK;
}

/* Allocates in the memory the given data */
mp_status_t mp_alloc(mem_pool_t *mp, void **out)
{
    if (mp == NULL || out == NULL)
    {
        return MP_ERR_NULL;
    }
    *out = NULL;

    if (mp->free_head == MP_INVALID)
    {
        return MP_ERR_EXHAUSTED;
    }

    uint16_t idx = mp->free_head;
    if (idx >= mp->block_count)
    {
        return MP_ERR_CORRUPT;
    }
    if (mp->in_use[idx] != 0)
    {
        return MP_ERR_CORRUPT;
    }

    uint16_t next = mp->next_free[idx];
    if (next != MP_INVALID && next >= mp->block_count)
    {
        return MP_ERR_CORRUPT;
    }

    mp->free_head   = next;
    mp->in_use[idx] = 1;
    mp->free_blocks--;
    *out = mp->storage + (size_t)(idx)*mp->block_size;

    return MP_OK;
}

/* Frees the given data from the memory pool */
mp_status_t mp_free(mem_pool_t *mp, void *block)
{
    if (mp == NULL || block == NULL)
    {
        return MP_ERR_NULL;
    }

    uintptr_t addr = (uintptr_t)block;
    uintptr_t base = (uintptr_t)mp->storage;
    if (addr < base)
    {
        return MP_ERR_BAD_POINTER;
    }

    size_t offset = addr - base;
    size_t total  = (size_t)mp->block_count * mp->block_size;
    if (offset >= total)
    {
        return MP_ERR_BAD_POINTER;
    }
    if (offset % mp->block_size != 0)
    {
        return MP_ERR_BAD_POINTER;
    }

    size_t idx = offset / mp->block_size;
    if (mp->in_use[idx] == 0)
    {
        return MP_ERR_DOUBLE_FREE;
    }
    if (mp->in_use[idx] != 1)
    {
        return MP_ERR_CORRUPT;
    }
    if (mp->free_blocks >= mp->block_count)
    {
        return MP_ERR_CORRUPT;
    }
    if (mp->free_head != MP_INVALID && mp->free_head >= mp->block_count)
    {
        return MP_ERR_CORRUPT;
    }

    mp->in_use[idx]    = 0;
    mp->next_free[idx] = mp->free_head;
    mp->free_head      = (uint16_t)idx;
    mp->free_blocks++;

    return MP_OK;
}

/* Returns the number of free blocks in the memory pool */
uint16_t mp_free_count(const mem_pool_t *mp)
{
    return mp == NULL ? 0 : mp->free_blocks;
}