# CommandParser
| Field    | Size      | Notes   |
|---------|-----------|----------|
| Sync   | 2 bytes | 0xA5 0x5A, marks the start of a packet   |
| Command ID     | 1 byte  | Which command |
| Length    | 1 byte      | Payload size, 0 to 32   |
| Payload   | 0-32 bytes | Command data   |
| CRC     | 2 bytes  | CRC-16 over Command ID, Length, and Payload |