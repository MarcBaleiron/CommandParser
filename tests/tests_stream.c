#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream.h"

#define STREAM_RING_CAPACITY 50
#define RANDOM_PACKET_COUNT  16

_Static_assert(STREAM_RING_CAPACITY >= PKT_MAX_LEN, "stream ring must fit a maximum packet");
_Static_assert((STREAM_RING_CAPACITY & (STREAM_RING_CAPACITY - 1)) != 0,
               "stream ring capacity must not be a power of two");

typedef struct
{
    ring_buffer_t  rb;
    uint8_t        storage[STREAM_RING_CAPACITY];
    stream_stats_t stats;
} stream_fixture_t;

#define CHECK(condition)                                                                 \
    do                                                                                   \
    {                                                                                    \
        if (!(condition))                                                                \
        {                                                                                \
            fprintf(stderr, "Test failed, %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return 0;                                                                    \
        }                                                                                \
    } while (0)

static int fixture_init(stream_fixture_t *fixture)
{
    memset(fixture, 0, sizeof *fixture);
    if (rb_init(&fixture->rb, fixture->storage, sizeof fixture->storage) != RB_OK)
    {
        fprintf(stderr, "Could not initialize stream test ring buffer\n");
        return 0;
    }
    return 1;
}

static int push_bytes(ring_buffer_t *rb, const uint8_t *bytes, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (rb_push(rb, bytes[i]) != RB_OK)
        {
            fprintf(stderr, "Could not push byte %zu into stream test ring buffer\n", i);
            return 0;
        }
    }
    return 1;
}

static int build_packet(uint8_t id, const uint8_t *payload, size_t payload_len, uint8_t *bytes, size_t *length)
{
    pkt_status_t status = packet_build(id, payload, payload_len, bytes, PKT_MAX_LEN, length);
    if (status != PKT_OK)
    {
        fprintf(stderr, "packet_build failed with status %d\n", (int)status);
        return 0;
    }
    return 1;
}

static int packet_matches(const packet_t *received, uint8_t id, const uint8_t *payload, size_t payload_len)
{
    if (received->id != id || received->len != payload_len || memcmp(received->payload, payload, payload_len) != 0)
    {
        fprintf(stderr,
                "Packet mismatch: expected id=%u len=%zu, received id=%u len=%u\n",
                (unsigned)id,
                payload_len,
                (unsigned)received->id,
                (unsigned)received->len);
        return 0;
    }
    return 1;
}

static int stats_invariant(const stream_stats_t *stats)
{
    const uint64_t errors = (uint64_t)stats->sync_errors + (uint64_t)stats->length_errors + (uint64_t)stats->crc_errors;
    if ((uint64_t)stats->discarded_bytes != errors)
    {
        fprintf(stderr,
                "Stats invariant failed: discarded=%u, sync=%u, length=%u, crc=%u\n",
                (unsigned)stats->discarded_bytes,
                (unsigned)stats->sync_errors,
                (unsigned)stats->length_errors,
                (unsigned)stats->crc_errors);
        return 0;
    }
    return 1;
}

static int test_garbage_then_valid(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t garbage[] = { 0x00, 0x11, 0x5A, 0xFF, 0x01 };
    const uint8_t payload[] = { 0x21, 0x22, 0x23 };
    uint8_t       bytes[PKT_MAX_LEN];
    size_t        length = 0;
    packet_t      out;

    CHECK(build_packet(0x31, payload, sizeof payload, bytes, &length));
    CHECK(push_bytes(&fixture.rb, garbage, sizeof garbage));
    CHECK(push_bytes(&fixture.rb, bytes, length));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_OK);
    CHECK(packet_matches(&out, 0x31, payload, sizeof payload));
    CHECK(fixture.stats.discarded_bytes == sizeof garbage);
    CHECK(fixture.stats.sync_errors == sizeof garbage);
    CHECK(fixture.stats.packets_ok == 1);
    CHECK(rb_count(&fixture.rb) == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int test_one_bad_byte_then_packet(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t bad_byte  = 0x00;
    const uint8_t payload[] = { 0x34, 0x35 };
    uint8_t       bytes[PKT_MAX_LEN];
    size_t        length = 0;
    packet_t      out;

    CHECK(build_packet(0x42, payload, sizeof payload, bytes, &length));
    CHECK(rb_push(&fixture.rb, bad_byte) == RB_OK);
    CHECK(push_bytes(&fixture.rb, bytes, length));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_OK);
    CHECK(packet_matches(&out, 0x42, payload, sizeof payload));
    CHECK(fixture.stats.sync_errors == 1);
    CHECK(fixture.stats.discarded_bytes == 1);
    CHECK(fixture.stats.packets_ok == 1);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int test_two_back_to_back(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t payload_a[] = { 0x10, 0x11 };
    const uint8_t payload_b[] = { 0x20, 0x21, 0x22 };
    uint8_t       bytes_a[PKT_MAX_LEN];
    uint8_t       bytes_b[PKT_MAX_LEN];
    size_t        length_a = 0;
    size_t        length_b = 0;
    packet_t      out;

    CHECK(build_packet(0x51, payload_a, sizeof payload_a, bytes_a, &length_a));
    CHECK(build_packet(0x52, payload_b, sizeof payload_b, bytes_b, &length_b));
    CHECK(push_bytes(&fixture.rb, bytes_a, length_a));
    CHECK(push_bytes(&fixture.rb, bytes_b, length_b));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_OK);
    CHECK(packet_matches(&out, 0x51, payload_a, sizeof payload_a));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_OK);
    CHECK(packet_matches(&out, 0x52, payload_b, sizeof payload_b));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_ERR_INCOMPLETE);
    CHECK(fixture.stats.packets_ok == 2);
    CHECK(rb_count(&fixture.rb) == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int test_corrupted_then_valid(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t payload_bad[]  = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 };
    const uint8_t payload_good[] = { 0x31, 0x32, 0x33 };
    uint8_t       bytes_bad[PKT_MAX_LEN];
    uint8_t       bytes_good[PKT_MAX_LEN];
    size_t        length_bad  = 0;
    size_t        length_good = 0;
    packet_t      out;

    CHECK(build_packet(0x61, payload_bad, sizeof payload_bad, bytes_bad, &length_bad));
    CHECK(build_packet(0x62, payload_good, sizeof payload_good, bytes_good, &length_good));
    for (size_t i = 1; i < length_bad; i++)
    {
        CHECK(bytes_bad[i] != PKT_SYNC0);
    }
    bytes_bad[PKT_HEADER_LEN] ^= 0x01;

    CHECK(push_bytes(&fixture.rb, bytes_bad, length_bad));
    CHECK(push_bytes(&fixture.rb, bytes_good, length_good));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_OK);
    CHECK(packet_matches(&out, 0x62, payload_good, sizeof payload_good));
    CHECK(fixture.stats.crc_errors == 1);
    CHECK(fixture.stats.sync_errors == length_bad - 1);
    CHECK(fixture.stats.discarded_bytes == length_bad);
    CHECK(fixture.stats.packets_ok == 1);
    CHECK(rb_count(&fixture.rb) == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int test_fake_sync_in_payload(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t payload[] = { 0x01, PKT_SYNC0, PKT_SYNC1, 0x02, 0x03 };
    uint8_t       bytes[PKT_MAX_LEN];
    size_t        length = 0;
    packet_t      out;

    CHECK(build_packet(0x71, payload, sizeof payload, bytes, &length));
    CHECK(push_bytes(&fixture.rb, bytes, length));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_OK);
    CHECK(packet_matches(&out, 0x71, payload, sizeof payload));
    CHECK(fixture.stats.packets_ok == 1);
    CHECK(fixture.stats.discarded_bytes == 0);
    CHECK(fixture.stats.sync_errors == 0);
    CHECK(fixture.stats.length_errors == 0);
    CHECK(fixture.stats.crc_errors == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int test_split_delivery(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t payload[] = { 0x41, 0x42, 0x43, 0x44 };
    uint8_t       bytes[PKT_MAX_LEN];
    size_t        length = 0;
    packet_t      out;

    CHECK(build_packet(0x72, payload, sizeof payload, bytes, &length));
    for (size_t i = 0; i < length; i++)
    {
        CHECK(rb_push(&fixture.rb, bytes[i]) == RB_OK);
        pkt_status_t status = stream_next_packet(&fixture.rb, &out, &fixture.stats);
        CHECK(status == (i + 1 == length ? PKT_OK : PKT_ERR_INCOMPLETE));
        CHECK(stats_invariant(&fixture.stats));
    }
    CHECK(packet_matches(&out, 0x72, payload, sizeof payload));
    CHECK(fixture.stats.packets_ok == 1);
    CHECK(rb_count(&fixture.rb) == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int test_wrap_around(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t payload[] = { 0x51, 0x52, 0x53, 0x54, 0x55 };
    uint8_t       bytes[PKT_MAX_LEN];
    size_t        length = 0;
    packet_t      out;
    const size_t  offset = STREAM_RING_CAPACITY - 3;

    CHECK(build_packet(0x73, payload, sizeof payload, bytes, &length));
    for (size_t i = 0; i < offset; i++)
    {
        CHECK(rb_push(&fixture.rb, 0x00) == RB_OK);
    }
    CHECK(rb_discard(&fixture.rb, offset) == RB_OK);
    CHECK(push_bytes(&fixture.rb, bytes, length));
    CHECK(fixture.rb.tail == offset);
    CHECK(fixture.rb.head == (offset + length) % STREAM_RING_CAPACITY);
    CHECK(fixture.rb.head < fixture.rb.tail);
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_OK);
    CHECK(packet_matches(&out, 0x73, payload, sizeof payload));
    CHECK(rb_count(&fixture.rb) == 0);
    CHECK(fixture.stats.discarded_bytes == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int check_noise_pattern(const uint8_t *pattern, size_t length)
{
    stream_fixture_t fixture;
    if (!fixture_init(&fixture) || !push_bytes(&fixture.rb, pattern, length))
    {
        return 0;
    }

    packet_t out;
    int      reached_incomplete = 0;
    for (size_t i = 0; i < STREAM_RING_CAPACITY + 1; i++)
    {
        pkt_status_t status = stream_next_packet(&fixture.rb, &out, &fixture.stats);
        if (status == PKT_ERR_INCOMPLETE)
        {
            reached_incomplete = 1;
            break;
        }
        if (status != PKT_OK)
        {
            fprintf(stderr, "Noise pattern returned unexpected status %d\n", (int)status);
            return 0;
        }
        fprintf(stderr, "Noise pattern unexpectedly produced a packet\n");
        return 0;
    }

    if (!reached_incomplete || rb_count(&fixture.rb) >= PKT_HEADER_LEN)
    {
        fprintf(stderr, "Noise pattern did not settle on an incomplete suffix\n");
        return 0;
    }
    return stats_invariant(&fixture.stats);
}

static int test_never_stuck(void)
{
    uint8_t zeros[STREAM_RING_CAPACITY];
    uint8_t ones[STREAM_RING_CAPACITY];
    uint8_t syncs[STREAM_RING_CAPACITY];
    uint8_t fake_syncs[STREAM_RING_CAPACITY];

    memset(zeros, 0x00, sizeof zeros);
    memset(ones, 0xFF, sizeof ones);
    memset(syncs, PKT_SYNC0, sizeof syncs);
    fake_syncs[0] = PKT_SYNC0;
    fake_syncs[1] = PKT_SYNC1;
    memset(fake_syncs + 2, PKT_SYNC1, sizeof fake_syncs - 2);

    CHECK(check_noise_pattern(zeros, sizeof zeros));
    CHECK(check_noise_pattern(ones, sizeof ones));
    CHECK(check_noise_pattern(syncs, sizeof syncs));
    CHECK(check_noise_pattern(fake_syncs, sizeof fake_syncs));
    return 1;
}

static int test_null_arguments(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    packet_t out;
    CHECK(stream_next_packet(NULL, &out, &fixture.stats) == PKT_ERR_NULL);
    CHECK(stream_next_packet(&fixture.rb, NULL, &fixture.stats) == PKT_ERR_NULL);
    CHECK(stream_next_packet(&fixture.rb, &out, NULL) == PKT_ERR_NULL);
    CHECK(fixture.stats.packets_ok == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static int test_out_untouched_on_incomplete(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    const uint8_t partial[] = { PKT_SYNC0, PKT_SYNC1, 0x81 };
    packet_t      out;
    packet_t      sentinel;
    memset(&sentinel, 0xC3, sizeof sentinel);
    memcpy(&out, &sentinel, sizeof out);

    CHECK(push_bytes(&fixture.rb, partial, sizeof partial));
    CHECK(stream_next_packet(&fixture.rb, &out, &fixture.stats) == PKT_ERR_INCOMPLETE);
    CHECK(memcmp(&out, &sentinel, sizeof out) == 0);
    CHECK(rb_count(&fixture.rb) == sizeof partial);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

static uint32_t random_next(uint32_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static uint8_t random_noise_byte(uint32_t *state)
{
    uint8_t byte = (uint8_t)random_next(state);
    while (byte == PKT_SYNC0)
    {
        byte = (uint8_t)random_next(state);
    }
    return byte;
}

static int collect_available(stream_fixture_t *fixture, packet_t *packets, size_t capacity, size_t *count)
{
    for (size_t i = 0; i < STREAM_RING_CAPACITY + 1; i++)
    {
        packet_t     out;
        pkt_status_t status = stream_next_packet(&fixture->rb, &out, &fixture->stats);
        if (status == PKT_ERR_INCOMPLETE)
        {
            return 1;
        }
        if (status != PKT_OK)
        {
            fprintf(stderr, "Random stream returned unexpected status %d\n", (int)status);
            return 0;
        }
        if (*count >= capacity)
        {
            fprintf(stderr, "Random stream produced more packets than expected\n");
            return 0;
        }
        packets[(*count)++] = out;
    }
    fprintf(stderr, "Random stream exceeded its processing iteration cap\n");
    return 0;
}

static int test_randomized(void)
{
    stream_fixture_t fixture;
    CHECK(fixture_init(&fixture));

    uint8_t  wire[RANDOM_PACKET_COUNT * (PKT_MAX_LEN + 5)];
    packet_t expected[RANDOM_PACKET_COUNT];
    packet_t received[RANDOM_PACKET_COUNT];
    size_t   wire_length    = 0;
    size_t   received_count = 0;
    uint32_t random_state   = 0x6D2B79F5u;

    for (size_t packet_index = 0; packet_index < RANDOM_PACKET_COUNT; packet_index++)
    {
        const size_t noise_length = 1 + (random_next(&random_state) % 5);
        for (size_t i = 0; i < noise_length; i++)
        {
            wire[wire_length++] = random_noise_byte(&random_state);
        }

        const size_t payload_length = 1 + (random_next(&random_state) % 12);
        uint8_t      payload[12];
        for (size_t i = 0; i < payload_length; i++)
        {
            payload[i] = (uint8_t)(packet_index * 13 + i + 1);
        }

        uint8_t       packet_bytes[PKT_MAX_LEN];
        size_t        packet_length = 0;
        const uint8_t id            = (uint8_t)(0x80 + packet_index);
        CHECK(build_packet(id, payload, payload_length, packet_bytes, &packet_length));
        expected[packet_index].id  = id;
        expected[packet_index].len = (uint8_t)payload_length;
        memcpy(expected[packet_index].payload, payload, payload_length);
        memcpy(wire + wire_length, packet_bytes, packet_length);
        wire_length += packet_length;
    }

    size_t wire_offset = 0;
    while (wire_offset < wire_length)
    {
        size_t chunk_length = 1 + (random_next(&random_state) % 10);
        if (chunk_length > wire_length - wire_offset)
        {
            chunk_length = wire_length - wire_offset;
        }
        CHECK(push_bytes(&fixture.rb, wire + wire_offset, chunk_length));
        wire_offset += chunk_length;
        CHECK(collect_available(&fixture, received, RANDOM_PACKET_COUNT, &received_count));
        CHECK(stats_invariant(&fixture.stats));
    }

    CHECK(collect_available(&fixture, received, RANDOM_PACKET_COUNT, &received_count));
    CHECK(received_count == RANDOM_PACKET_COUNT);
    for (size_t i = 0; i < RANDOM_PACKET_COUNT; i++)
    {
        CHECK(packet_matches(&received[i], expected[i].id, expected[i].payload, expected[i].len));
    }
    CHECK(fixture.stats.packets_ok == RANDOM_PACKET_COUNT);
    CHECK(rb_count(&fixture.rb) == 0);
    CHECK(stats_invariant(&fixture.stats));
    return 1;
}

int main(void)
{
    int all_passed = 1;

    all_passed &= test_garbage_then_valid();
    all_passed &= test_one_bad_byte_then_packet();
    all_passed &= test_two_back_to_back();
    all_passed &= test_corrupted_then_valid();
    all_passed &= test_fake_sync_in_payload();
    all_passed &= test_split_delivery();
    all_passed &= test_wrap_around();
    all_passed &= test_never_stuck();
    all_passed &= test_null_arguments();
    all_passed &= test_out_untouched_on_incomplete();
    all_passed &= test_randomized();

    if (all_passed)
    {
        printf("All stream tests passed.\n");
    }
    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}