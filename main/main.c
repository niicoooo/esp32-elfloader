#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_heap_caps_init.h"


static esp_err_t event_handler(void *ctx, system_event_t *event) {
    (void) ctx;
    (void) event;
    return ESP_OK;
}


static const char* TAG = "main";
void register_system();
void register_wget();



void app_main(void) {
    ESP_LOGI(TAG, "Let's go!\n");
    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    ESP_LOGI(TAG, "Configuration done!\n");

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );
    linenoiseSetMultiLine(1);
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);
    linenoiseHistorySetMaxLen(100);

    esp_console_register_help_command();
    register_system();
    register_wget();
    ESP_LOGI(TAG, "Console configuration done");

    const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;
    while(true) {
        char* line = linenoise(prompt);
        if (line == NULL) {
            continue;
        }
        linenoiseHistoryAdd(line);
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x\n", ret);
        } else if (err != ESP_OK) {
            printf("Internal error: 0x%x\n", err);
        }
        linenoiseFree(line);
    }
}
