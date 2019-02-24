#include <stdint.h>
#include <stdint.h>


extern uint32_t data;

intptr_t local_main(intptr_t arg) {
    return data;
}
