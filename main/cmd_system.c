#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"

#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "esp_spi_flash.h"
#include "rom/tbconsole.h"


static void register_free();
static void register_restart();
static void register_deep_sleep();
static void register_printtime();
static void register_version();
static void register_setloglevel();



void register_system() {
    register_free();
    register_restart();
    register_deep_sleep();
    register_printtime();
    register_version();
    register_setloglevel();
}

/** 'restart' command restarts the program */

static int restart(int argc, char** argv) {
    ESP_LOGI(__func__, "Restarting");
    esp_restart();
}

static void register_restart() {
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Restart the program",
        .hint = NULL,
        .func = &restart,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/** 'free' command prints available heap memory */

static int free_mem(int argc, char** argv) {
    printf("%d\n", esp_get_free_heap_size());
    return 0;
}

static void register_free() {
    const esp_console_cmd_t cmd = {
        .command = "free",
        .help = "Get the total size of heap memory available",
        .hint = NULL,
        .func = &free_mem,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/** 'deep_sleep' command puts the chip into deep sleep mode */

static struct {
    struct arg_int *wakeup_time;
    struct arg_int *wakeup_gpio_num;
    struct arg_int *wakeup_gpio_level;
    struct arg_end *end;
} deep_sleep_args;


static int deep_sleep(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**) &deep_sleep_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, deep_sleep_args.end, argv[0]);
        return 1;
    }
    if (deep_sleep_args.wakeup_time->count) {
        uint64_t timeout = 1000ULL * deep_sleep_args.wakeup_time->ival[0];
        ESP_LOGI(__func__, "Enabling timer wakeup, timeout=%lluus", timeout);
        ESP_ERROR_CHECK( esp_sleep_enable_timer_wakeup(timeout) );
    }
    if (deep_sleep_args.wakeup_gpio_num->count) {
        int io_num = deep_sleep_args.wakeup_gpio_num->ival[0];
        if (!rtc_gpio_is_valid_gpio(io_num)) {
            ESP_LOGE(__func__, "GPIO %d is not an RTC IO", io_num);
            return 1;
        }
        int level = 0;
        if (deep_sleep_args.wakeup_gpio_level->count) {
            level = deep_sleep_args.wakeup_gpio_level->ival[0];
            if (level != 0 && level != 1) {
                ESP_LOGE(__func__, "Invalid wakeup level: %d", level);
                return 1;
            }
        }
        ESP_LOGI(__func__, "Enabling wakeup on GPIO%d, wakeup on %s level",
                 io_num, level ? "HIGH" : "LOW");

        ESP_ERROR_CHECK( esp_sleep_enable_ext1_wakeup(1ULL << io_num, level) );
    }
    esp_deep_sleep_start();
}

static void register_deep_sleep() {
    deep_sleep_args.wakeup_time =
        arg_int0("t", "time", "<t>", "Wake up time, ms");
    deep_sleep_args.wakeup_gpio_num =
        arg_int0(NULL, "io", "<n>",
                 "If specified, wakeup using GPIO with given number");
    deep_sleep_args.wakeup_gpio_level =
        arg_int0(NULL, "io_level", "<0|1>", "GPIO level to trigger wakeup");
    deep_sleep_args.end = arg_end(3);

    const esp_console_cmd_t cmd = {
        .command = "sleep",
        .help = "Enter deep sleep mode. "
        "Two wakeup modes are supported: timer and GPIO. "
        "If no wakeup option is specified, will sleep indefinitely.",
        .hint = NULL,
        .func = &deep_sleep,
        .argtable = &deep_sleep_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/** 'time' command prints current time */

static int printtime(int argc, char** argv) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("%f\n", tv.tv_sec + tv.tv_usec / 1000000.0);
    return 0;
}

static void register_printtime() {
    const esp_console_cmd_t cmd = {
        .command = "time",
        .help = "Display current time",
        .hint = NULL,
        .func = &printtime,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}


/** 'version' command displays firmware version */

static int version(int argc, char** argv) {
    printf("%s %s\n", __DATE__, __TIME__);
    return 0;
}

static void register_version() {
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Display firmware version",
        .hint = NULL,
        .func = &version,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

/** 'setloglevem' command sets log level */

static struct {
    struct arg_str *tag;
    struct arg_int *level;
    struct arg_end *end;
} loglevel_args;

static int setloglevel(int argc, char** argv) {
    int nerrors = arg_parse(argc, argv, (void**) &loglevel_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, loglevel_args.end, argv[0]);
        return 1;
    }
    esp_log_level_set(loglevel_args.tag->sval[0], loglevel_args.level->ival[0]);
    return 0;
}

static void register_setloglevel() {
    loglevel_args.tag = arg_str1(NULL, NULL, "<tag>", "Module tag filter");
    loglevel_args.level = arg_int1(NULL, NULL, "<n>", "Log level");
    loglevel_args.end = arg_end(5);

    const esp_console_cmd_t cmd = {
        .command = "setloglevel",
        .help = "Test",
        .hint = NULL,
        .func = &setloglevel,
        .argtable = &loglevel_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
