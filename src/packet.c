#include "packet.h"

pkt_status_t packet_parse(const uint8_t *buf, size_t buf_len,
                            packet_t *out, size_t *consumed)
{
    (void)buf;
    (void)buf_len;
    (void)out;
    (void)consumed;

    return PKT_OK;
}