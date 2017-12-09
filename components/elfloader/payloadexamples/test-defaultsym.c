#include <stdint.h>
#include <stdint.h>


extern void abort();

uint32_t local_main(uint32_t arg) {
    return (uint32_t)&abort;
}
