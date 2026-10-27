#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "packet.h"

int main(void)
{
    const uint8_t payload[] = {0x10, 0x20, 0x30};
    uint8_t buf[PKT_HEADER_LEN + PKT_MAX_PAYLOAD + PKT_CRC_LEN];
    size_t written = 99;

    assert(packet_build(0x42, payload, sizeof payload, buf, sizeof buf, &written) == PKT_OK);
    assert(written == PKT_HEADER_LEN + sizeof payload + PKT_CRC_LEN);

    packet_t packet = {0};
    size_t consumed = 99;

    assert(packet_parse(NULL, written, &packet, &consumed) == PKT_ERR_NULL);
    assert(packet_parse(buf, written, NULL, &consumed) == PKT_ERR_NULL);
    assert(packet_parse(buf, written, &packet, NULL) == PKT_ERR_NULL);

    consumed = 99;
    assert(packet_parse(buf, PKT_HEADER_LEN - 1, &packet, &consumed) == PKT_ERR_INCOMPLETE);
    assert(consumed == 0);

    consumed = 99;
    assert(packet_parse(buf, written - 1, &packet, &consumed) == PKT_ERR_INCOMPLETE);
    assert(consumed == 0);

    uint8_t invalid_buf[sizeof buf];
    memcpy(invalid_buf, buf, written);
    invalid_buf[0] = 0;
    assert(packet_parse(invalid_buf, written, &packet, &consumed) == PKT_ERR_BAD_SYNC);

    memcpy(invalid_buf, buf, written);
    invalid_buf[3] = PKT_MAX_PAYLOAD + 1;
    assert(packet_parse(invalid_buf, written, &packet, &consumed) == PKT_ERR_BAD_LENGTH);

    memcpy(invalid_buf, buf, written);
    invalid_buf[written - 1] ^= 0x01;
    assert(packet_parse(invalid_buf, written, &packet, &consumed) == PKT_ERR_BAD_CRC);

    memset(packet.payload, 0xFF, sizeof packet.payload);
    assert(packet_parse(buf, written, &packet, &consumed) == PKT_OK);
    assert(consumed == written);
    assert(packet.id == 0x42);
    assert(packet.len == sizeof payload);
    assert(memcmp(packet.payload, payload, sizeof payload) == 0);
    for (size_t i = sizeof payload; i < sizeof packet.payload; ++i) {
        assert(packet.payload[i] == 0);
    }

    assert(packet_build(0x42, payload, sizeof payload, NULL, sizeof buf, &written) == PKT_ERR_NULL);
    assert(packet_build(0x42, payload, sizeof payload, buf, sizeof buf, NULL) == PKT_ERR_NULL);
    assert(packet_build(0x42, NULL, sizeof payload, buf, sizeof buf, &written) == PKT_ERR_NULL);
    assert(packet_build(0x42, payload, PKT_MAX_PAYLOAD + 1, buf, sizeof buf, &written) ==
           PKT_ERR_BAD_LENGTH);
    assert(packet_build(0x42, payload, sizeof payload, buf,
                        PKT_HEADER_LEN + sizeof payload + PKT_CRC_LEN - 1, &written) ==
           PKT_ERR_BUF_TOO_SMALL);

    written = 99;
    assert(packet_build(0x42, NULL, 0, buf, sizeof buf, &written) == PKT_OK);
    assert(written == PKT_HEADER_LEN + PKT_CRC_LEN);

    puts("packet tests passed");
    return 0;
}