#include "ring_buffer.h"

/* Initializes the ring buffer with the given storage and capacity */
rb_status_t rb_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity)
{
    if (rb == NULL || storage == NULL)
    {
        return RB_ERR_NULL;
    }

    if (capacity == 0)
    {
        return RB_ERR_RANGE;
    }

    rb->storage  = storage;
    rb->capacity = capacity;
    rb->head = rb->tail = rb->count = 0;

    return RB_OK;
}

/* Pushes a byte into the end of the ring buffer */
rb_status_t rb_push(ring_buffer_t *rb, uint8_t byte)
{
    if (rb == NULL || rb->storage == NULL)
    {
        return RB_ERR_NULL;
    }
    if (rb->capacity == 0)
    {
        return RB_ERR_RANGE;
    }
    if (rb->count > rb->capacity)
    {
        return RB_ERR_RANGE;
    }
    if (rb->count >= rb->capacity)
    {
        return RB_ERR_FULL;
    }

    rb->storage[rb->head] = byte;
    rb->head              = (rb->head + 1) % rb->capacity;
    rb->count++;

    return RB_OK;
}

/* Removes the oldest byte from the ring buffer. */
rb_status_t rb_pop(ring_buffer_t *rb, uint8_t *byte)
{
    if (rb == NULL || byte == NULL || rb->storage == NULL)
    {
        return RB_ERR_NULL;
    }
    if (rb->capacity == 0)
    {
        return RB_ERR_RANGE;
    }
    if (rb->count > rb->capacity)
    {
        return RB_ERR_RANGE;
    }
    if (rb->count == 0)
    {
        return RB_ERR_EMPTY;
    }

    *byte    = rb->storage[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;

    return RB_OK;
}

/* Copy up to len bytes (all or nothing) starting at offset, without removing them. */
rb_status_t rb_peek(const ring_buffer_t *rb, size_t offset, uint8_t *out, size_t len)
{
    if (rb == NULL || out == NULL || rb->storage == NULL)
    {
        return RB_ERR_NULL;
    }
    if (rb->capacity == 0)
    {
        return RB_ERR_RANGE;
    }
    if (rb->count > rb->capacity)
    {
        return RB_ERR_RANGE;
    }
    if (offset > rb->count || len > rb->count - offset)
    {
        return RB_ERR_RANGE;
    }
    if (len == 0)
    {
        return RB_OK;
    }

    size_t start = (rb->tail + offset) % rb->capacity;
    for (size_t i = 0; i < len; i++)
    {
        out[i] = rb->storage[(start + i) % rb->capacity];
    }

    return RB_OK;
}

/* Drop n bytes from the front. */
rb_status_t rb_discard(ring_buffer_t *rb, size_t n)
{
    if (rb == NULL || rb->storage == NULL)
    {
        return RB_ERR_NULL;
    }
    if (rb->capacity == 0)
    {
        return RB_ERR_RANGE;
    }
    if (rb->count > rb->capacity)
    {
        return RB_ERR_RANGE;
    }
    if (n > rb->count)
    {
        return RB_ERR_RANGE;
    }

    rb->tail   = (rb->tail + n) % rb->capacity;
    rb->count -= n;

    return RB_OK;
}

/* Return the count of the given ring buffer */
size_t rb_count(const ring_buffer_t *rb)
{
    return rb == NULL || rb->count > rb->capacity ? 0 : rb->count;
}

/* Return the free capacity of the given ring buffer */
size_t rb_free(const ring_buffer_t *rb)
{
    return rb == NULL || rb->count > rb->capacity ? 0 : rb->capacity - rb->count;
}