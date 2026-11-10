#ifndef STREAM_H
#define STREAM_H

#include "packet.h"
#include "ring_buffer.h"

typedef struct
{
    uint32_t packets_ok;
    uint32_t sync_errors;
    uint32_t length_errors;
    uint32_t crc_errors;
    uint32_t discarded_bytes;
} stream_stats_t;

/* Try to extract one packet from the ring buffer.
 * Returns PKT_OK and fills *out if a valid packet was found and removed.
 * Returns PKT_ERR_INCOMPLETE if more bytes are needed (nothing else to do).
 * Returns PKT_ERR_INTERNAL when encountering an internal error.
 * Bad data is discarded and counted internally; it never returns bad-data errors. */
pkt_status_t stream_next_packet(ring_buffer_t *rb, packet_t *out, stream_stats_t *stats);

#endif