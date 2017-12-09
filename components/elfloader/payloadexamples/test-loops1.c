#include <stdio.h>
#include <stdint.h>


uint32_t local_main(uint32_t arg) {
    for (int i = 0; i < 10; i++) {
        printf("%i...\n", i);
    }
    printf("Hello world!\n");
    return 0;
}
