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
#include "../components/qrlib/include/quirc_internal.h"
#include "lwip/api.h"
#include "camera.h"
#include "bitmap.h"
#include "iot_lcd.h"
#include "iot_wifi_conn.h"
#include "app_camera.h"

#if CONFIG_USE_LCD
CEspLcd* tft = NULL;

typedef struct {
    uint8_t frame_num;
}camera_evt_t;

QueueHandle_t camera_queue = NULL;
static const char* TAG = "ESP-CAM-LCD";

void queue_send(uint8_t frame_num)
{
    camera_evt_t camera_event;
    camera_event.frame_num = frame_num;
    xQueueSend(camera_queue, &camera_event, portMAX_DELAY);
}

uint8_t queue_receive()
{
    camera_evt_t camera_event;
    xQueueReceive(camera_queue, &camera_event, portMAX_DELAY);
    return camera_event.frame_num;
}

uint8_t queue_available()
{
    return uxQueueSpacesAvailable(camera_queue);
}

void app_lcd_task(void *pvParameters)
{
    uint8_t i = 0;
    uint32_t time = 0;
    camera_evt_t camera_event;
    camera_event.frame_num = 0;
    time = xTaskGetTickCount();

    while(1) {
         xQueueReceive(camera_queue, &camera_event, portMAX_DELAY);
         if((xTaskGetTickCount() - time) > 1000 / portTICK_RATE_MS ) {
             ESP_LOGI(TAG,"app_lcd_task movie %d  fps", i);
             time = xTaskGetTickCount();
             i = 0;
         }
         i++;
        tft->drawBitmap(0, 0, (uint16_t*)currFbPtr[camera_event.frame_num], 320, 240, false);
//         tft->drawBitmap(0, 0, (uint16_t*)camera_get_fb(camera_event.frame_num), 320, 240, false);
        // vTaskDelay(5000 / portTICK_PERIOD_MS);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void app_lcd_init()
{
    lcd_conf_t lcd_pins = {
        .lcd_model    = LCD_MOD_ST7789,
        .pin_num_miso = GPIO_NUM_25,//GPIO_NUM_12,//GPIO_NUM_25,
        .pin_num_mosi = GPIO_NUM_23,//GPIO_NUM_13,//GPIO_NUM_23,
        .pin_num_clk  = GPIO_NUM_19,//GPIO_NUM_14,//GPIO_NUM_19,
        .pin_num_cs   = GPIO_NUM_22,//GPIO_NUM_15,//GPIO_NUM_22,
        .pin_num_dc   = GPIO_NUM_21,//GPIO_NUM_16,//GPIO_NUM_21,
        .pin_num_rst  = GPIO_NUM_18,//GPIO_NUM_33,//GPIO_NUM_18,
        .pin_num_bckl = GPIO_NUM_5,//GPIO_NUM_17,//GPIO_NUM_5,
        .clk_freq     = 20 * 1000 * 1000,
        .rst_active_level = 0,
        .bckl_active_level = 0,
        .spi_host = HSPI_HOST,
    };

    /*Initialize SPI Handler*/
    if (tft == NULL) {
        tft = new CEspLcd(&lcd_pins);
        camera_queue = xQueueCreate(CAMERA_CACHE_NUM - 2, sizeof(camera_evt_t));
    }

    /*screen initialize*/
    tft->invertDisplay(false);
    tft->setRotation(3);
    tft->fillScreen(COLOR_GREEN);
    tft->drawBitmap(0, 0, esp_logo, 137, 26);
    tft->drawString("Status: Initialize camera ...", 5, 30);
    tft->drawString("Status: Wifi Connecting ...", 5, 44);
}
#endif
