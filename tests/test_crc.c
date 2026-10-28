#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "crc16.h"

int main(void)
{
    const char *s   = "123456789";
    uint16_t    crc = crc16_ccitt((const uint8_t *)s, strlen(s));
    assert(crc == 0x29B1);

    assert(crc16_ccitt(NULL, 0) == 0xFFFF); /* empty input */

    printf("crc tests passed\n");
    return 0;
}