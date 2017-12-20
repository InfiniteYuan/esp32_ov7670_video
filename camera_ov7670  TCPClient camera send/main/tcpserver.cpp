/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "lwip/api.h"
#include "camera.h"
#include "bitmap.h"
#include "iot_lcd.h"
#include "iot_tcp.h"
#include "iot_wifi_conn.h"
#include "app_camera.h"
#include "nvs_flash.h"
#include "esp_wifi_internal.h"
#include "string.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"

//#define SERVER_PORT CONFIG_TCP_SERVER_PORT
//#define SERVER_MAX_CONNECTION  CONFIG_TCP_SERVER_MAX_CONNECTION

static const char* TAG_SRV = "TCP_SRV";
static EventGroupHandle_t wifi_event_group;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            break;
        default:
            break;
    }
    return ESP_OK;
}

void initialise_wifi(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
    ;

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t wifi_config;
    memcpy(wifi_config.ap.ssid, "123456789", sizeof("123456789"));
    memcpy(wifi_config.ap.password, "123456789", sizeof("123456789"));
    wifi_config.ap.ssid_len = strlen("123456789");
    wifi_config.ap.max_connection = 1;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    esp_wifi_start();
}

void tcp_client_obj_task(void *pvParameters)
{
    CTcpConn client;
    uint8_t num = 0;
    uint8_t * data;

    if (client.Connect("192.168.4.1", 8080) < 0) {
        ESP_LOGI(TAG_SRV, "fail to connect...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    while (1) {
        num = queue_receive();
        data = (uint8_t*) camera_get_fb(num);
        for (int j = 0; j < 150; j++) {
            ESP_LOGI(TAG_SRV, "send %d data:%d", j, client.Write((const uint8_t *) data, 1024));
//            vTaskDelay(100 / portTICK_PERIOD_MS);
            data += 1024;
        }
//        if (client.Write((const uint8_t *)data, 320*240*2) < 0) {
//            vTaskDelay(5000 / portTICK_PERIOD_MS);
//            ESP_LOGI(TAG_SRV, "fail to send data...");
//        } else {
//            ESP_LOGI(TAG_SRV, "send data success...");
//            for(int i=0;i<20;i++){
//                ESP_LOGI(TAG_SRV, "buffer 0x%0x", data[i]);
//            }
//        }
    }
}
