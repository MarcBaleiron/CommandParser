#include <string.h>

#include "packet.h"
#include "crc16.h"

/* Wire layout:
 *   [0]=SYNC0 [1]=SYNC1 [2]=ID [3]=LEN [4..4+LEN-1]=payload [CRC_hi][CRC_lo]
 * CRC is CRC-16/CCITT-FALSE over ID, LEN, and payload, sent big-endian
 * (high byte first). packet_build MUST use the same order. */
#define PKT_ID_OFFSET        2
#define PKT_LEN_OFFSET       3
#define PKT_CRC_INPUT_OFFSET 2 /* CRC starts at ID, right after the sync bytes */

pkt_status_t packet_parse(const uint8_t *buf, size_t buf_len, packet_t *out, size_t *consumed)
{
    if (buf == NULL || out == NULL || consumed == NULL)
    {
        return PKT_ERR_NULL;
    }
    *consumed = 0; /* Defined value for failure cases */

    if (buf_len < PKT_HEADER_LEN)
    {
        return PKT_ERR_INCOMPLETE;
    }

    if (buf[0] != PKT_SYNC0 || buf[1] != PKT_SYNC1)
    {
        return PKT_ERR_BAD_SYNC;
    }

    const uint8_t len = buf[PKT_LEN_OFFSET];
    if (len > PKT_MAX_PAYLOAD)
    {
        return PKT_ERR_BAD_LENGTH;
    }

    const size_t total = (size_t)PKT_HEADER_LEN + (size_t)len + (size_t)PKT_CRC_LEN;
    if (buf_len < total)
    {
        return PKT_ERR_INCOMPLETE;
    }

    /* CRC input = ID + LEN + payload = 2 + len bytes */
    const size_t   crc_input_len  = 2u + (size_t)len;
    const uint16_t calculated_crc = crc16_ccitt(buf + PKT_CRC_INPUT_OFFSET, crc_input_len);

    /* Merges the high and low bit from the received CRC */
    const size_t   crc_pos      = (size_t)PKT_HEADER_LEN + (size_t)len;
    const uint16_t received_crc = (uint16_t)(((uint16_t)buf[crc_pos] << 8) | (uint16_t)buf[crc_pos + 1]);

    if (calculated_crc != received_crc)
    {
        return PKT_ERR_BAD_CRC;
    }

    /* Everything validated */
    out->id  = buf[PKT_ID_OFFSET];
    out->len = len;
    memset(out->payload, 0, sizeof out->payload); /* Clears possible leftovers */
    memcpy(out->payload, buf + PKT_HEADER_LEN, len);

    *consumed = total;

    return PKT_OK;
}

pkt_status_t packet_build(uint8_t        id,
                          const uint8_t *payload,
                          size_t         payload_len,
                          uint8_t       *out_buf,
                          size_t         out_capacity,
                          size_t        *written)
{
    if (out_buf == NULL || written == NULL)
    {
        return PKT_ERR_NULL;
    }
    *written = 0; /* Defined value for failure cases */

    if (payload_len > PKT_MAX_PAYLOAD)
    {
        return PKT_ERR_BAD_LENGTH;
    }

    if (payload_len > 0 && payload == NULL)
    {
        return PKT_ERR_NULL;
    }

    const size_t total = (size_t)PKT_HEADER_LEN + (size_t)payload_len + (size_t)PKT_CRC_LEN;
    if (out_capacity < total)
    {
        return PKT_ERR_BUF_TOO_SMALL;
    }

    out_buf[0]              = PKT_SYNC0;
    out_buf[1]              = PKT_SYNC1;
    out_buf[PKT_ID_OFFSET]  = id;
    out_buf[PKT_LEN_OFFSET] = (uint8_t)payload_len;
    if (payload_len > 0)
    {
        memcpy(out_buf + PKT_HEADER_LEN, payload, payload_len);
    }

    /* CRC input = ID + LEN + payload = 2 + payload_len bytes */
    const uint16_t calculated_crc = crc16_ccitt(out_buf + PKT_CRC_INPUT_OFFSET, (size_t)2 + payload_len);

    /* Takes the high and low bits from the calculated CRC and writes them into their respective locations */
    const size_t crc_pos = (size_t)PKT_HEADER_LEN + (size_t)payload_len;
    out_buf[crc_pos]     = ((uint8_t)(calculated_crc >> 8));
    out_buf[crc_pos + 1] = ((uint8_t)(calculated_crc & 0xFF));

    *written = total;

    return PKT_OK;
}