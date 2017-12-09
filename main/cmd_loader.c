#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "loader.h"
#include "payloadexamples/all.h"


void* modules_data[] = {
    test_argvalue_elf, test_defaultsym_elf, test_nop_elf, test_printf_longcall_elf,
    test_printf_O3_elf, test_printf_Os_elf, test_printf_sections_elf, test_printf_shortcall_elf,
    test_printf_stripped_elf, test_return_bss_elf, test_return_rwdata_elf, test_returnvalue_elf,
    test_printf_multiplefuncs_elf, test_printf_multiplestrings_elf, test_loops1_elf, test_loops2_elf
};
char* modules_name[] = {
    "test_argvalue_elf", "test_defaultsym_elf", "test_nop_elf", "test_printf_longcall_elf",
    "test_printf_O3_elf", "test_printf_Os_elf", "test_printf_sections_elf", "test_printf_shortcall_elf",
    "test_printf_stripped_elf", "test_return_bss_elf", "test_return_rwdata_elf", "test_returnvalue_elf",
    "test_printf_multiplefuncs_elf", "test_printf_multiplestrings_elf", "test_loops1_elf", "test_loops2_elf"
};

static const ELFLoaderSymbol_t exports[] = {
    { "puts", (void*) puts }
};
static const ELFLoaderEnv_t env = { exports, sizeof(exports) / sizeof(*exports) };

static struct {
    struct arg_int *module_number;
    struct arg_end *end;
} run_args;

static int cmd_run(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**) &run_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, run_args.end, argv[0]);
        return 1;
    }
    int n = run_args.module_number->ival[0];
    if (n <=0 || n > (sizeof(modules_data) / sizeof (void*))) {
        printf("Modules:\n");
        for (int i = 0; i < (sizeof(modules_data) / sizeof (void*)); i ++) {
            printf("  %2i %s\n", i+1, modules_name[i]);
        }
        return -1;
    }
    printf("Running: %s(0x10)\n", modules_name[n-1]);
    int r = elfLoader(modules_data[n-1], &env, "local_main", 0x10);
    printf("Result: 0x%X\n", r);
    return 0;
}

static void register_run() {
    run_args.module_number = arg_int1(NULL, NULL, "<module example number>", "Module example number");
    run_args.end = arg_end(5);
    const esp_console_cmd_t cmd = {
        .command = "run",
        .help = "Load and run elf module",
        .hint = NULL,
        .func = &cmd_run,
        .argtable = &run_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_runall(int argc, char** argv) {
    for (int i = 0; i < (sizeof(modules_data) / sizeof (void*)); i ++) {
        ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(modules_data[i], &env);
        if (ctx == NULL) {
            printf("%2i %-40s InitLoadAndRelocate failed\n", i+1, modules_name[i]);
            continue;
        }
        int r = elfLoaderSetFunc(ctx, "local_main");
        if (r != 0) {
            printf("%2i %-40s SetFunc failed\n", i+1, modules_name[i]);
        } else {
            printf("%2i %-40s OK\n", i+1, modules_name[i]);
        }
        elfLoaderFree(ctx);
    }
    return 0;
}

static void register_runall() {
    const esp_console_cmd_t cmd = {
        .command = "runall",
        .help = "Load and test all modules",
        .hint = NULL,
        .func = &cmd_runall,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void register_loader() {
    register_run();
    register_runall();
}
