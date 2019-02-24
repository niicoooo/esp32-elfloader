#include <stdint.h>
#include <stdint.h>


uint32_t data;

intptr_t local_main(intptr_t arg) {
    if (arg == 1) {
        data = 0x12345678;
    } else {
        data = 0;
    }
    return data;
}
