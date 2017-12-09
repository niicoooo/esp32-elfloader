#include <stdint.h>
#include <stdint.h>


volatile uint32_t data = 0x12345678;

uint32_t local_main(uint32_t arg) {
    return data;
}
