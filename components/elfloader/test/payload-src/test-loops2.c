#include <stdio.h>
#include <stdint.h>


intptr_t local_main(intptr_t arg) {
    int i;
    for (i = 10; i > 0; i--) {
        printf("%i...\n", i);
    }
    return 0;
}
