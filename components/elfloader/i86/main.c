#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>


#include "../loader.h"


#define APP_NAME "../payloads/test-printf-longcall.elf"


static const ELFLoaderSymbol_t exports[] = { { "puts", (void*) 1 } };
static const ELFLoaderEnv_t env = { exports, sizeof(exports) / sizeof(*exports) };

int main( int argc, char *argv[] ) {
    FILE* fd;
    if ( argc >= 2 ) {
        fd = fopen(argv[1], "rb");
    } else {
        fd = fopen(APP_NAME, "rb");
    }

    if (fd == NULL) {
        if ( argc >= 2 ) {
            printf("Error opening: %s\n", argv[1]);
        } else {
            printf("Error opening: %s\n", APP_NAME);
        }
        return -1;
    }

    ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(fd, &env);
    if (!ctx) {
        fclose(fd);
        return -1;
    }
    if (elfLoaderSetFunc(ctx, "local_main") != 0) {
        elfLoaderFree(ctx);
        fclose(fd);
        return -1;
    }
    elfLoaderFree(ctx);
    fclose(fd);
    return 0;
}
