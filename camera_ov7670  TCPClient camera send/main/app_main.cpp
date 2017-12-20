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

void app_camera_init()
{
    camera_model_t camera_model;
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CONFIG_D0;
    config.pin_d1 = CONFIG_D1;
    config.pin_d2 = CONFIG_D2;
    config.pin_d3 = CONFIG_D3;
    config.pin_d4 = CONFIG_D4;
    config.pin_d5 = CONFIG_D5;
    config.pin_d6 = CONFIG_D6;
    config.pin_d7 = CONFIG_D7;
    config.pin_xclk = CONFIG_XCLK;
    config.pin_pclk = CONFIG_PCLK;
    config.pin_vsync = CONFIG_VSYNC;
    config.pin_href = CONFIG_HREF;
    config.pin_sscb_sda = CONFIG_SDA;
    config.pin_sscb_scl = CONFIG_SCL;
    config.pin_reset = CONFIG_RESET;
    config.xclk_freq_hz = CONFIG_XCLK_FREQ;

    //Warning: This gets squeezed into IRAM.
    volatile static uint32_t * * currFbPtr __attribute__ ((aligned(4))) = NULL;
    currFbPtr= (volatile uint32_t **)malloc(sizeof(uint32_t *) * CAMERA_CACHE_NUM);

    ESP_LOGI(TAG, "get free size of 32BIT heap : %d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT));

    for(int i = 0; i < CAMERA_CACHE_NUM; i++){
        currFbPtr[i] = (volatile uint32_t *) heap_caps_malloc(320 * 240 * 2, MALLOC_CAP_32BIT);
        ESP_LOGI(TAG, "framebuffer address is:%p\n", currFbPtr[i]);
    }

    ESP_LOGI(TAG, "%s\n", currFbPtr[0] == NULL ? "currFbPtr is NULL" : "currFbPtr not NULL");
    ESP_LOGI(TAG, "framebuffer address is:%p\n", currFbPtr[0]);

    // camera init
    esp_err_t err = camera_probe(&config, &camera_model);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera probe failed with error 0x%x", err);
        return;
    }

    if (camera_model == CAMERA_OV7670) {
        ESP_LOGI(TAG, "Detected OV7670 camera");
        config.frame_size = CAMERA_FRAME_SIZE;
    } else {
        ESP_LOGI(TAG, "Cant detected ov7670 camera");
    }

    config.displayBuffer = (uint32_t**) currFbPtr;
    config.pixel_format = CAMERA_PIXEL_FORMAT;
//    config.test_pattern_enabled = CONFIG_ENABLE_TEST_PATTERN;

    err = camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
}

static void app_camera_task(void *pvParameters)
{
    uint32_t i = 0;
    uint8_t frame_num = 0;
    uint32_t time = 0;
    time = xTaskGetTickCount();
    while (1) {
//        queue_send(i%CAMERA_CACHE_NUM);
//        if((xTaskGetTickCount() - time) > 1000 / portTICK_RATE_MS ){
//            ESP_LOGI(TAG,"app_camera_task movie %d fps", i);
//            time = xTaskGetTickCount();
//            i = 0;
//        }
//        i++;
        if ((frame_num = camera_run()) != ESP_FAIL) {
            ESP_LOGI(TAG,"camera_run OK");
            queue_send(frame_num % CAMERA_CACHE_NUM);
        }
    }
}

extern "C" void app_main()
{
    app_camera_init();
#if CONFIG_USE_LCD
    app_lcd_init();
//    uint8_t * imgbuf = (uint8_t *)heap_caps_malloc(320*240, MALLOC_CAP_8BIT);
#endif

    CWiFi *my_wifi = CWiFi::GetInstance(WIFI_MODE_STA);
    printf("connect wifi\n");
    my_wifi->Connect(WIFI_SSID, WIFI_PASSWORD, portMAX_DELAY);

    // VERY UNSTABLE without this delay after init'ing wifi... however, much more stable with a new Power Supply
    vTaskDelay(500 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "Free heap: %u", xPortGetFreeHeapSize());

//    ESP_LOGD(TAG, "Starting http_server task...");
//    xTaskCreatePinnedToCore(&http_server_task, "http_server_task", 4096, NULL, 5, NULL, 1);

    xTaskCreate(&tcp_client_obj_task, "tcp_client_obj_task", 4096, NULL, 4, NULL);
#if CONFIG_USE_LCD
//    ESP_LOGD(TAG, "Starting app_lcd_task...");
//    xTaskCreate(&app_lcd_task, "app_lcd_task", 4096, NULL, 4, NULL);
#endif

    ESP_LOGD(TAG, "Starting app_camera_task...");
    xTaskCreate(&app_camera_task, "app_camera_task", 4096, NULL, 3, NULL);

    ip4_addr_t s_ip_addr = my_wifi->IP();
    ESP_LOGI(TAG, "open http://" IPSTR "/bmp for single image/bitmap image", IP2STR(&s_ip_addr));
    ESP_LOGI(TAG, "open http://" IPSTR "/get for raw image as stored in framebuffer ", IP2STR(&s_ip_addr));

    ESP_LOGI(TAG, "get free size of 32BIT heap : %d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT));
    ESP_LOGI(TAG, "task stack: %d", uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI(TAG, "Camera demo ready.");
}
