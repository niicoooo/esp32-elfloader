#include <stdint.h>
#include <stdint.h>


volatile uint32_t data = 0x12345678;

intptr_t local_main(intptr_t arg) {
    return data;
}
