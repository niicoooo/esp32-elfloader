#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "loader.h"
#include "payload.h"



static const char* TAG = "main";


static const ELFLoaderSymbol_t exports[] = {
    { "puts", (void*) puts },
    { "printf", (void*) printf },
};
static const ELFLoaderEnv_t env = { exports, sizeof(exports) / sizeof(*exports) };


void app_main(void) {
    ESP_LOGI(TAG, "Let's go!\n");

    ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(example_payload_payload_elf, &env);
    if (!ctx) {
        ESP_LOGI(TAG, "elfLoaderInitLoadAndRelocate error");
        return;
    }
    if (elfLoaderSetFunc(ctx, "local_main") != 0) {
        ESP_LOGI(TAG, "elfLoaderSetFunc error: local_main function not fount");
        elfLoaderFree(ctx);
        return;
    }
    ESP_LOGI(TAG, "Running local_main(0x10) function as int local_main(int arg)");
    int r = elfLoaderRun(ctx, 0x10);
    ESP_LOGI(TAG, "Result: %i", r);
    elfLoaderFree(ctx);

    ESP_LOGI(TAG, "Done!\n");
    return;

}
