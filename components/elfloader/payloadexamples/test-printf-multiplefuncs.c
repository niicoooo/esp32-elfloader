#include <stdio.h>
#include <stdint.h>


uint32_t local_main1(uint32_t arg) {
    printf("Other 1!\n");
    return 0;
}

uint32_t local_main2(uint32_t arg) {
    printf("Other 2!\n");
    return 0;
}

uint32_t local_main3(uint32_t arg) {
    printf("Other 3!\n");
    return 0;
}

uint32_t local_main(uint32_t arg) {
    printf("Hello world!\n");
    return 0;
}
