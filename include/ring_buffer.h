#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *storage;  /* caller-provided memory */
    size_t   capacity; /* size of storage in bytes */
    size_t   head;     /* next write index */
    size_t   tail;     /* next read index */
    size_t   count;    /* bytes currently stored */
} ring_buffer_t;

typedef enum
{
    RB_OK = 0,
    RB_ERR_NULL,
    RB_ERR_FULL,
    RB_ERR_EMPTY,
    RB_ERR_RANGE
} rb_status_t;

rb_status_t rb_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity);
rb_status_t rb_push(ring_buffer_t *rb, uint8_t byte);
rb_status_t rb_pop(ring_buffer_t *rb, uint8_t *byte);

/* Copy len bytes (all or nothing) starting at offset, without removing them. */
rb_status_t rb_peek(const ring_buffer_t *rb, size_t offset, uint8_t *out, size_t len);

/* Drop n bytes from the front. */
rb_status_t rb_discard(ring_buffer_t *rb, size_t n);

size_t rb_count(const ring_buffer_t *rb);
size_t rb_free(const ring_buffer_t *rb);

#endif