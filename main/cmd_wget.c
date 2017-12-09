#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "loader.h"
#include "tcpip_adapter.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "mdns.h"
#include "mongoose.h"

#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "driver/uart.h"



static int initdone = 0;
static mdns_server_t * mdns = NULL;
struct mg_mgr mgr;
static int exit_flag;
static void* data;
static int size;
static const char *url = CONFIG_URL;
static const char* TAG = "wifi";

static const ELFLoaderSymbol_t exports[] = {
    { "puts", (void*) puts },
    { "memcpy", (void*) memcpy },
    { "memmove", (void*) memmove },
    { "strcmp", (void*) strcmp },
    { "strtol", (void*) strtol },
    { "mktime", (void*) mktime },
    { "xTaskCreatePinnedToCore", (void*) xTaskCreatePinnedToCore },
    { "vTaskDelay", (void*) vTaskDelay },
    { "uart_param_config", (void*) uart_param_config },
    { "uart_set_pin", (void*) uart_set_pin },
    { "uart_driver_install", (void*) uart_driver_install },
    { "uart_read_bytes", (void*) uart_read_bytes },
    { "esp_log_timestamp", (void*) esp_log_timestamp },
    { "esp_log_write", (void*) esp_log_write },
    { "strlen", (void*) strlen },
    { "putchar", (void*) putchar },
    { "uart_write_bytes", (void*) uart_write_bytes },
};

static const ELFLoaderEnv_t env = { exports, sizeof(exports) / sizeof(*exports) };

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    (void) ctx;
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "ip: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
        ESP_LOGI(TAG, "netmask: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.netmask));
        ESP_LOGI(TAG, "gw: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.gw));
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void ev_handler(struct mg_connection *c, int ev, void *p) {
    if (ev == MG_EV_HTTP_REPLY) {
        struct http_message *hm = (struct http_message *)p;
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
        ESP_LOGI(TAG, "MG_EV_HTTP_REPLY (response code: %i, size: %i)", hm->resp_code, hm->body.len);
        if (hm->resp_code == 200) {
            size = hm->body.len;
            data = malloc(hm->body.len);
            assert(data);
            memcpy(data, hm->body.p, hm->body.len);
        }
        exit_flag = 1;
    } else if (ev == MG_EV_CLOSE) {
        ESP_LOGI(TAG, "MG_EV_CLOSE");
        exit_flag = 1;
    };
}

static int cmd_wget(int argc, char** argv) {
    if (!initdone) {
        ESP_LOGI(TAG, "Starting wifi...");
        esp_event_loop_set_cb(event_handler, NULL);
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
        ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
        ESP_ERROR_CHECK( esp_wifi_start() );
        ESP_LOGI(TAG, "Connecting ap as a sta...");
        wifi_config_t wifi_config = { 0 };
        strncpy((char*) wifi_config.sta.ssid, CONFIG_WIFI_SSID, sizeof(wifi_config.sta.ssid));
        strncpy((char*) wifi_config.sta.password, CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_connect() );
        ESP_ERROR_CHECK( mdns_init(TCPIP_ADAPTER_IF_STA, &mdns) );
        mdns_set_hostname(mdns, "esp32");
        mdns_set_instance(mdns, "ESP32 module");
        ESP_LOGI(TAG, "Wifi started!");
        initdone = 1;
    }

    mg_mgr_init(&mgr, NULL);
    exit_flag = 0;
    data = 0;
    ESP_LOGI(TAG, "mg_connect_http...");
    mg_connect_http(&mgr, ev_handler, url, NULL, NULL);
    while (exit_flag == 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
    ESP_LOGI(TAG, "mg_connect_http done");
    if (data) {
//        ESP_LOG_BUFFER_HEXDUMP(TAG, data, size, 3);
        printf("Running module\n");

        ELFLoaderContext_t* ctx = elfLoaderInitLoadAndRelocate(data, &env);
        if (!ctx) {
            return -1;
        }
        if (elfLoaderSetFunc(ctx, "local_main") != 0) {
            elfLoaderFree(ctx);
            return -1;
        }
        elfLoaderRun(ctx, 0x10);
// elfLoaderFree(ctx);
    }
    return 0;
}

static void register_cmd_wget() {
    const esp_console_cmd_t cmd = {
        .command = "wget",
        .help = "Download and run elf module",
        .hint = NULL,
        .func = &cmd_wget,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void register_wget() {
    register_cmd_wget();
}
