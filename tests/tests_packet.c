#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "packet.h"
#include "crc16.h"

/* Switch for translating error numbers into names */
static const char *pkt_status_name(pkt_status_t status)
{
    switch (status)
    {
        case PKT_OK:
            return "PKT_OK";
        case PKT_ERR_NULL:
            return "PKT_ERR_NULL";
        case PKT_ERR_INCOMPLETE:
            return "PKT_ERR_INCOMPLETE";
        case PKT_ERR_BAD_SYNC:
            return "PKT_ERR_BAD_SYNC";
        case PKT_ERR_BAD_LENGTH:
            return "PKT_ERR_BAD_LENGTH";
        case PKT_ERR_BAD_CRC:
            return "PKT_ERR_BAD_CRC";
        case PKT_ERR_BUF_TOO_SMALL:
            return "PKT_ERR_BUF_TOO_SMALL";
        default:
            return "UNKNOWN_PKT_STATUS";
    }
}

static int check_at(pkt_status_t received, pkt_status_t expected, const char *file, int line)
{
    if (received == expected)
    { /* If equal return nothing */
        return 1;
    }
    else
    { /* If not equal return file, line and expected and received values */
        fprintf(stderr,
                "Test failed, File: %s, Line: %d, Expected: %s, Received: %s.\n",
                file,
                line,
                pkt_status_name(expected),
                pkt_status_name(received));
        return 0;
    }
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check(received, expected) check_at((received), (expected), __FILE__, __LINE__)

int main(void)
{
    int all_passed = 1;

    /* ----------packet_parse tests---------- */

    /* Null arguments */
    const uint8_t buf_null[PKT_HEADER_LEN] = { 0 };
    packet_t      out_null;
    size_t        consumed_null = 0;

    all_passed &= check(packet_parse(NULL, sizeof buf_null, &out_null, &consumed_null), PKT_ERR_NULL);
    all_passed &= check(packet_parse(buf_null, sizeof buf_null, NULL, &consumed_null), PKT_ERR_NULL);
    all_passed &= check(packet_parse(buf_null, sizeof buf_null, &out_null, NULL), PKT_ERR_NULL);

    /* Buffer length smaller than header length */
    const uint8_t buf_inc_1[PKT_HEADER_LEN] = { 4 };
    packet_t      out_inc_1;
    size_t        consumed_inc_1 = 6;

    all_passed &= check(packet_parse(buf_inc_1, 3, &out_inc_1, &consumed_inc_1), PKT_ERR_INCOMPLETE);

    /* One or both incorrect sync bytes */
    const uint8_t buf_sync_1[PKT_HEADER_LEN] = { PKT_SYNC0,
                                                 0x00, /* Expected PKT_SYNC1 */
                                                 0,
                                                 0 };
    const uint8_t buf_sync_2[PKT_HEADER_LEN] = { 0x00, /* Expected PKT_SYNC0 */
                                                 PKT_SYNC1,
                                                 0,
                                                 0 };
    const uint8_t buf_sync_3[PKT_HEADER_LEN] = { 0x00, /* Expected PKT_SYNC0 */
                                                 0x00, /* Expected PKT_SYNC1 */
                                                 0,
                                                 0 };
    packet_t      out_sync;
    size_t        consumed_sync = 0;

    all_passed &= check(packet_parse(buf_sync_1, sizeof buf_sync_1, &out_sync, &consumed_sync), PKT_ERR_BAD_SYNC);
    all_passed &= check(packet_parse(buf_sync_2, sizeof buf_sync_2, &out_sync, &consumed_sync), PKT_ERR_BAD_SYNC);
    all_passed &= check(packet_parse(buf_sync_3, sizeof buf_sync_3, &out_sync, &consumed_sync), PKT_ERR_BAD_SYNC);

    /* Packet length exceeds max payload */
    const uint8_t buf_len[PKT_HEADER_LEN] = { PKT_SYNC0, PKT_SYNC1, 0, PKT_MAX_PAYLOAD + 1 };
    packet_t      out_len;
    size_t        consumed_len = 39;

    all_passed &= check(packet_parse(buf_len, sizeof buf_len, &out_len, &consumed_len), PKT_ERR_BAD_LENGTH);

    /* Total packet exceeds buffer */
    const uint8_t buf_inc_2[PKT_HEADER_LEN] = { 0 };
    packet_t      out_inc_2;
    size_t        consumed_inc_2 = 0;

    all_passed &=
        check(packet_parse(buf_inc_2, (sizeof buf_inc_2) - 1, &out_inc_2, &consumed_inc_2), PKT_ERR_INCOMPLETE);

    /* Incorrect CRC */
    uint8_t buf_crc[PKT_HEADER_LEN + PKT_MAX_PAYLOAD + PKT_CRC_LEN] = { PKT_SYNC0, PKT_SYNC1, 0, PKT_MAX_PAYLOAD };

    const uint16_t calculated_crc  = crc16_ccitt(buf_crc + 2, (size_t)2 + PKT_MAX_PAYLOAD);
    const size_t   pos_crc         = (size_t)PKT_HEADER_LEN + (size_t)PKT_MAX_PAYLOAD;
    buf_crc[pos_crc]               = ((uint8_t)(calculated_crc >> 8));
    buf_crc[pos_crc + 1]           = ((uint8_t)(calculated_crc & 0xFF));
    buf_crc[pos_crc]              ^= 0x01; /* Last bit flipped */

    packet_t out_crc;
    size_t   consumed_crc = 0;

    all_passed &= check(packet_parse(buf_crc, sizeof buf_crc, &out_crc, &consumed_crc), PKT_ERR_BAD_CRC);

    /* ----------packet_build tests---------- */

    /* Null arguments */
    uint8_t output_null_1[PKT_HEADER_LEN + PKT_CRC_LEN];
    size_t  written_null_1 = 0;

    all_passed &= check(packet_build(1, NULL, 0, NULL, sizeof output_null_1, &written_null_1), PKT_ERR_NULL);
    all_passed &= check(packet_build(1, NULL, 0, output_null_1, sizeof output_null_1, NULL), PKT_ERR_NULL);
    all_passed &= check(packet_build(1, NULL, 1, output_null_1, sizeof output_null_1, &written_null_1), PKT_ERR_NULL);

    /* Payload length exceeds max length */
    uint8_t output_len[PKT_HEADER_LEN + PKT_CRC_LEN];
    size_t  written_len = 0;

    all_passed &= check(packet_build(1, NULL, 33, output_len, sizeof output_len, &written_len), PKT_ERR_BAD_LENGTH);

    /* Null payload */
    uint8_t output_null_2[PKT_HEADER_LEN + PKT_CRC_LEN];
    size_t  written_null_2 = 0;

    all_passed &= check(packet_build(1, NULL, 1, output_null_2, sizeof output_null_2, &written_null_2), PKT_ERR_NULL);

    /* Total exceeds capacity */
    const uint8_t payload_small[PKT_MAX_PAYLOAD] = { 0 };
    uint8_t       output_small[PKT_HEADER_LEN + PKT_CRC_LEN];
    size_t        written_small = 0;

    all_passed &=
        check(packet_build(1, payload_small, sizeof payload_small, output_small, sizeof output_small, &written_small),
              PKT_ERR_BUF_TOO_SMALL);

    /* Returns wether all the tests have passed or not */
    if (all_passed == 1)
    {
        printf("All tests passed correctly.\n");
    }
    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}