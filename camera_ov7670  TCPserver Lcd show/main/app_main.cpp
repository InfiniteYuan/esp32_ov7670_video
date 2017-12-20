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
#include "iot_wifi_conn.h"
#include "app_camera.h"

static const char* TAG = "ESP-CAM";

extern "C" void app_main()
{
    initialise_wifi();
    initialise_buffer();
    app_lcd_init();

    xTaskCreate(&tcp_server_task, "tcp_server_task", 4096, NULL, 4, NULL);
    xTaskCreate(&app_lcd_task, "app_lcd_task", 4096, NULL, 4, NULL);
    // tcpip_adapter_ip_info_t ipconfig;
    // tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ipconfig);

    // ESP_LOGI(TAG, "TCP connect...");
    // ESP_LOGI(TAG, "connect to: %d.%d.%d.%d(%d)", ((uint8_t*)&ipconfig.ip.addr)[0], ((uint8_t*)&ipconfig.ip.addr)[1],
    //                                                  ((uint8_t*)&ipconfig.ip.addr)[2], ((uint8_t*)&ipconfig.ip.addr)[3], 8080);
    // ESP_LOGI(TAG, "sizeof(size_t):%d", sizeof(size_t));
//    app_camera_init();
//#if CONFIG_USE_LCD
//    app_lcd_init();
////    uint8_t * imgbuf = (uint8_t *)heap_caps_malloc(320*240, MALLOC_CAP_8BIT);
//#endif
//
//    CWiFi *my_wifi = CWiFi::GetInstance(WIFI_MODE_STA);
//    printf("connect wifi\n");
//    my_wifi->Connect(WIFI_SSID, WIFI_PASSWORD, portMAX_DELAY);
//
//    // VERY UNSTABLE without this delay after init'ing wifi... however, much more stable with a new Power Supply
//    vTaskDelay(500 / portTICK_RATE_MS);
//    ESP_LOGI(TAG, "Free heap: %u", xPortGetFreeHeapSize());
//
//    ESP_LOGD(TAG, "Starting http_server task...");
//    xTaskCreatePinnedToCore(&http_server_task, "http_server_task", 4096, NULL, 5, NULL, 1);
//
//#if CONFIG_USE_LCD
//    ESP_LOGD(TAG, "Starting app_lcd_task...");
//    xTaskCreate(&app_lcd_task, "app_lcd_task", 4096, NULL, 4, NULL);
//#endif
//
//    ESP_LOGD(TAG, "Starting app_camera_task...");
//    xTaskCreate(&app_camera_task, "app_camera_task", 4096, NULL, 3, NULL);
//
//    ip4_addr_t s_ip_addr = my_wifi->IP();
//    ESP_LOGI(TAG, "open http://" IPSTR "/bmp for single image/bitmap image", IP2STR(&s_ip_addr));
//    ESP_LOGI(TAG, "open http://" IPSTR "/get for raw image as stored in framebuffer ", IP2STR(&s_ip_addr));
//
//    ESP_LOGI(TAG, "get free size of 32BIT heap : %d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT));
//    ESP_LOGI(TAG, "task stack: %d", uxTaskGetStackHighWaterMark(NULL));
//    ESP_LOGI(TAG, "Camera demo ready.");
}
