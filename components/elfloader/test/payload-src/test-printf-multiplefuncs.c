#include <stdio.h>
#include <stdint.h>


intptr_t local_main1(intptr_t arg) {
    printf("Other 1!\n");
    return 0;
}

intptr_t local_main2(intptr_t arg) {
    printf("Other 2!\n");
    return 0;
}

intptr_t local_main3(intptr_t arg) {
    printf("Other 3!\n");
    return 0;
}

intptr_t local_main(intptr_t arg) {
    printf("Hello world!\n");
    return 0;
}
