#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "loader.h"
#include "../payload-build/test-printf-multiplestrings-obj.h"



static const ELFLoaderSymbol_t exports[] = {
    { "puts", (void*) puts }
};
static const ELFLoaderEnv_t env = { exports, sizeof(exports) / sizeof(*exports) };



TEST_CASE("test-printf-multiplestrings", "[esp32-elfloader]") {

    ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(payload_build_test_printf_multiplestrings_elf, &env);
    if (ctx == NULL) {
        printf("InitLoadAndRelocate failed\n");
        TEST_ASSERT( 0 );
        return;
    }
    int r = elfLoaderSetFunc(ctx, "local_main");
    if (r != 0) {
        printf("SetFunc failed\n");
        elfLoaderFree(ctx);
        TEST_ASSERT( 0 );
        return;
    }
    intptr_t result = elfLoaderRun(ctx, 0x00 );
    elfLoaderFree(ctx);
    TEST_ASSERT( 1 );
    TEST_ASSERT( result == 0x00 );

}
