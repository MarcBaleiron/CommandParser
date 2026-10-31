# CommandParser
| Field    | Size      | Notes   |
|---------|-----------|----------|
| Sync   | 2 bytes | 0xA5 0x5A, marks the start of a packet   |
| Command ID     | 1 byte  | Which command |
| Length    | 1 byte      | Payload size, 0 to 32   |
| Payload   | 0-32 bytes | Command data   |
| CRC     | 2 bytes  | CRC-16 over Command ID, Length, and Payload |

## Ring buffer

The ring buffer rejects a byte when full and returns `RB_ERR_FULL`; it never
overwrites older bytes. This avoids silently corrupting a command packet.
It tracks the number of stored bytes in `count` because `head == tail` can
mean either that the buffer is empty or that it is full. Indices wrap at the
storage capacity.

`rb_count(NULL)` and `rb_free(NULL)` return 0.