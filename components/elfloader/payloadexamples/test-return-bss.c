#include <stdint.h>
#include <stdint.h>


volatile uint32_t data;

uint32_t local_main(uint32_t arg) {
    data = 0x12345678;
    return data;
}
