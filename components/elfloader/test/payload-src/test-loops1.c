#include <stdio.h>
#include <stdint.h>


intptr_t local_main(intptr_t arg) {
    int i;
    for (i = 0; i < 10; i++) {
        printf("%i...\n", i);
    }
    return i;
}
