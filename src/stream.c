#include "stream.h"

pkt_status_t stream_next_packet(ring_buffer_t *rb, packet_t *out, stream_stats_t *stats)
{
    if (rb == NULL || out == NULL || stats == NULL)
    {
        return PKT_ERR_NULL;
    }

    size_t  budget   = rb_count(rb);
    size_t  consumed = 0;
    uint8_t scratch[PKT_MAX_LEN];

    /* Each iteration returns or removes one byte.
     * The budget starts at the byte count.
     * A successful parse consumes at least 6 bytes. */
    for (;;)
    {
        if (budget == 0)
        {
            return PKT_ERR_INCOMPLETE;
        }
        budget--;

        size_t available = rb_count(rb);
        if (available == 0)
        {
            return PKT_ERR_INCOMPLETE;
        }

        size_t n = (available < PKT_MAX_LEN) ? available : PKT_MAX_LEN;
        if (rb_peek(rb, 0, scratch, n) != RB_OK)
        {
            return PKT_ERR_INTERNAL;
        }

        switch (packet_parse(scratch, n, out, &consumed))
        {
            case PKT_OK:
                if (rb_discard(rb, consumed) != RB_OK)
                {
                    return PKT_ERR_INTERNAL;
                }
                stats->packets_ok++;
                return PKT_OK;
            case PKT_ERR_INCOMPLETE:
                return PKT_ERR_INCOMPLETE;
            case PKT_ERR_BAD_SYNC:
                stats->sync_errors++;
                break;
            case PKT_ERR_BAD_LENGTH:
                stats->length_errors++;
                break;
            case PKT_ERR_BAD_CRC:
                stats->crc_errors++;
                break;
            default: /* PKT_ERR_NULL, BUF_TOO_SMALL, and INTERNAL fall into default */
                return PKT_ERR_INTERNAL;
        }

        if (rb_discard(rb, 1) != RB_OK)
        {
            return PKT_ERR_INTERNAL;
        }

        stats->discarded_bytes++;
    }
}