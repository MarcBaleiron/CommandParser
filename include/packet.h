#ifndef PACKET_H
#define PACKET_H

#include <stddef.h>
#include <stdint.h>

#define PKT_SYNC0       0xA5
#define PKT_SYNC1       0x5A
#define PKT_MAX_PAYLOAD 32
#define PKT_HEADER_LEN  4   /* sync(2) + id(1) + len(1) */
#define PKT_CRC_LEN     2

typedef enum {
    PKT_OK = 0,
    PKT_ERR_NULL,
    PKT_ERR_INCOMPLETE,   /* Needs more bytes */
    PKT_ERR_BAD_SYNC,
    PKT_ERR_BAD_LENGTH,
    PKT_ERR_BAD_CRC,
    PKT_ERR_BUF_TOO_SMALL
} pkt_status_t;

typedef struct {
    uint8_t id;
    uint8_t len;
    uint8_t payload[PKT_MAX_PAYLOAD];
} packet_t;

/* Parse one packet from buf. On PKT_OK, *consumed = bytes used. */
pkt_status_t packet_parse(const uint8_t *buf, size_t buf_len,
                            packet_t *out, size_t *consumed);

/* Parse one packet from id, payload and payload_len. On PKT_OK, *written = bytes created. */
pkt_status_t packet_build(uint8_t id, const uint8_t *payload, size_t payload_len,
                            uint8_t *out_buf, size_t out_capacity, size_t *written);
#endif