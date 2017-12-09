#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "unaligned.h"


TEST_CASE("unalignedSet8 & unalignedGet8", "[elfLoader-utils]") {
    uint8_t c[128];

    memset(c, 0, 128);
    unalignedSet8(&c[4], 0x01);
    unalignedSet8(&c[5], 0x02);
    unalignedSet8(&c[6], 0x03);
    unalignedSet8(&c[7], 0x04);

    TEST_ASSERT( c[4] == 0x01 );
    TEST_ASSERT( c[5] == 0x02 );
    TEST_ASSERT( c[6] == 0x03 );
    TEST_ASSERT( c[7] == 0x04 );

    TEST_ASSERT( unalignedGet8(&c[4]) == 0x01 );
    TEST_ASSERT( unalignedGet8(&c[5]) == 0x02 );
    TEST_ASSERT( unalignedGet8(&c[6]) == 0x03 );
    TEST_ASSERT( unalignedGet8(&c[7]) == 0x04 );
}
