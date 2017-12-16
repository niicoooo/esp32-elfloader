# elfloader module for esp32

This esp32 component is able to load and relocate elf module.

### ELF creation

See [moduleexample](components/elfloader/moduleexample) and [payloadexamples](components/elfloader/payloadexamples) folders for some examples.

The code must be compiled with `-fno-common` option and linked with `-Wl,-r -nostartfiles -nodefaultlibs -nostdlib` options

### Usage

Example:

```c
#include "loader.h"


char* data = ... Elf module ...

static const ELFLoaderSymbol_t exports[] = {
    { "puts", (void*) puts },
    { "memcpy", (void*) memcpy },
    { "memmove", (void*) memmove },
    { "strcmp", (void*) strcmp },
    { "strtol", (void*) strtol },
    ...
};
static const ELFLoaderEnv_t env = { exports, sizeof(exports) / sizeof(*exports) };


ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(data, &env);
if (!ctx) {
    ESP_LOGI(TAG, "elfLoaderInitLoadAndRelocate error");
    return -1;
}
if (elfLoaderSetFunc(ctx, "local_main") != 0) {
    ESP_LOGI(TAG, "elfLoaderSetFunc error: local_main function not fount");
    elfLoaderFree(ctx);
    return -1;
}
ESP_LOGI(TAG, "Running local_main(0x10) function as int local_main(int arg)");
int r = elfLoaderRun(ctx, 0x10);
ESP_LOGI(TAG, "Result: %i", r);
elfLoaderFree(ctx);
return 0;
```

or

```bash
$ make && make flash && make monitor
esp32> run 4
Running: test_printf_longcall_elf(0x10)
Hello world!
Result: 0x0
```
