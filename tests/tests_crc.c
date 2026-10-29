#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "crc16.h"

/* Function for checking if the values coincide */
static int check_at(uint16_t received, uint16_t expected, const char *file, int line)
{
    if (received == expected)
    { /* If equal return nothing */
        return 1;
    }
    else
    { /* If not equal return file, line and expected and received values */
        fprintf(stderr,
                "Test failed, File: %s, Line: %d, Expected: 0x%04X, Received: 0x%04X.\n",
                file,
                line,
                (unsigned int)expected,
                (unsigned int)received);
        return 0;
    }
}

/* Used a macros so that "__FILE__" and "__LINE__" work properly */
#define check(received, expected) check_at((received), (expected), __FILE__, __LINE__)

int main(void)
{
    int all_passed = 1;

    /* Correct CRC encoding */
    const char *s     = "123456789";
    uint16_t    crcS  = crc16_ccitt((const uint8_t *)s, strlen(s));
    all_passed       &= check(crcS, 0x29B1);

    /* Empty input */
    all_passed &= check(crc16_ccitt(NULL, 0), 0xFFFF);

    /* Single byte: "A" */
    const uint8_t single_byte[]  = { 'A' };
    all_passed                  &= check(crc16_ccitt(single_byte, sizeof single_byte), 0xB915);

    /* Embedded zero: all three bytes, including the bytes after 0x00, are processed */
    const uint8_t embedded_zero[]  = { 'A', 0x00, 'B' };
    all_passed                    &= check(crc16_ccitt(embedded_zero, sizeof embedded_zero), 0x8E87);

    /* Returns wether all the tests have passed or not */
    if (all_passed == 1)
    {
        printf("All tests passed correctly.\n");
    }
    return all_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}