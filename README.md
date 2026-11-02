# CommandParser

A C11 project for building and parsing command packets. It includes a CRC-16/CCITT-FALSE checksum implementation and a fixed-capacity ring buffer for holding incoming bytes.

## Packet format

| Field | Size | Description |
|---|---:|---|
| Sync | 2 bytes | `0xA5 0x5A` |
| Command ID | 1 byte | Identifies the command |
| Length | 1 byte | Payload size, 0–32 bytes |
| Payload | 0–32 bytes | Command data |
| CRC | 2 bytes | CRC-16 over command ID, length, and payload |

The packet module provides functions to build and parse packets. The CRC module calculates CRC-16/CCITT-FALSE (polynomial `0x1021`, initial value `0xFFFF`).

## Ring buffer

The ring buffer stores bytes in FIFO order and wraps indices at its capacity. It tracks the number of stored bytes because equal head and tail indices can mean either full or empty.

When full, it rejects new bytes with `RB_ERR_FULL`; it does not overwrite existing data. `rb_count(NULL)` and `rb_free(NULL)` return 0.

## Memory pool

The memory pool uses `uint16_t` block indices, with `MP_INVALID` (`0xFFFF`) reserved as the free-list terminator. Therefore, the maximum supported `block_count` is 65535, giving valid block indices from 0 through 65534. Ensure any count is within this range before converting it to the `uint16_t` parameter; a count of 65536 cannot be represented.

## Build and test

```sh
cmake -S . -B build
cmake --build build

# Run all tests together
ctest --test-dir build --output-on-failure

# Run one test file
./build/tests_crc
./build/tests_packet
./build/tests_rb