#include "crc16.h"

uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF; /* Remainder starts at all ones */
    for (size_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8); /* Shifts the byte to the high half */
        for (int b = 0; b < 8; b++)
        {
            if (crc & 0x8000)
            { /* If the top bit is 1 divide by the polynomial */
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            }
            else
            { /* Else shift up by 1 bit */
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}