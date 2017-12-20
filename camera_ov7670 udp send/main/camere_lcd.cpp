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

void convert(unsigned char *img565, unsigned char *imgGray, int iWidth, int iHeight)
{
    uint16_t * pData565;
    pData565 = (uint16_t *)img565;
    int iIndex = 0;
    for (int x = 0; x < iHeight; ++x)
    {
        for (int y = 0; y < iWidth; ++y)
        {
//            unsigned char chR = *(pData565 + iIndex) & RGB565_MASK_RED >> 11;
//            unsigned char chG = *(pData565 + iIndex) & RGB565_MASK_GREEN >> 5;
//            unsigned char chB = *(pData565 + iIndex) & RGB565_MASK_BLUE;
            unsigned char chR = ((*(pData565 + iIndex) & RGB565_MASK_RED) >> 11 ) << 3;
            unsigned char chG = ((*(pData565 + iIndex) & RGB565_MASK_GREEN) >> 5) << 2;
            unsigned char chB = (*(pData565 + iIndex) & RGB565_MASK_BLUE) << 3;
            unsigned char chGray = (chR*30 + chG*59 + chB*11 + 50) / 100;//(chB*0.3 +chR*0.11 +chG*0.59);
            *(imgGray + x * iWidth + y) = chGray;
            ++iIndex;
        }
    }
}

void qrdecode(uint8_t j, uint8_t * imgbuf)
{
    struct quirc *qr;
    int num_codes;
    int i;
    uint8_t *image;
    int w, h;

    qr = quirc_new();
    if (!qr) {
        perror("Failed to allocate memory");
        abort();
    }

    if (quirc_resize(qr, 320, 240, imgbuf) < 0) {
        perror("Failed to allocate video memory");
        abort();
    }

    image = quirc_begin(qr, &w, &h);
    quirc_end(qr);
//    convert((unsigned char *)camera_get_fb(j), image, 320, 240);
//    num_codes = quirc_count(qr);
//    for (i = 0; i < num_codes; i++) {
//        struct quirc_code code;
//        struct quirc_data data;
//        quirc_decode_error_t err;
//
//        quirc_extract(qr, i, &code);
//
//        /* Decoding stage */
//        err = quirc_decode(&code, &data);
//        if (err)
//            printf("DECODE FAILED: %s\n", quirc_strerror(err));
//        else
//            printf("Data: %s\n", data.payload);
//    }
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
//            qrdecode(camera_event.frame_num, (uint8_t *)pvParameters);
            i = 0;
        }
        i++;
        tft->drawBitmap(0, 0, (uint16_t *)camera_get_fb(camera_event.frame_num), camera_get_fb_width(), camera_get_fb_height(), false);
    }
}

void app_lcd_init()
{
    camera_queue = xQueueCreate(CAMERA_CACHE_NUM - 1, sizeof(camera_evt_t));
//    lcd_conf_t lcd_pins = {
//        .lcd_model = LCD_MOD_ST7789,  //LCD_MOD_ILI9341,//LCD_MOD_ST7789,
//        .pin_num_miso = CONFIG_HW_LCD_MISO_GPIO,
//        .pin_num_mosi = CONFIG_HW_LCD_MOSI_GPIO,
//        .pin_num_clk = CONFIG_HW_LCD_CLK_GPIO,
//        .pin_num_cs = CONFIG_HW_LCD_CS_GPIO,
//        .pin_num_dc = CONFIG_HW_LCD_DC_GPIO,
//        .pin_num_rst = CONFIG_HW_LCD_RESET_GPIO,
//        .pin_num_bckl = CONFIG_HW_LCD_BL_GPIO,
//        .clk_freq = 40 * 1000 * 1000,
//        .rst_active_level = 0,
//        .bckl_active_level = 0,
//        .spi_host = HSPI_HOST,};
//
//    /*Initialize SPI Handler*/
//    if (tft == NULL) {
//        tft = new CEspLcd(&lcd_pins);
//        camera_queue = xQueueCreate(CAMERA_CACHE_NUM - 1, sizeof(camera_evt_t));
//    }
//
//    /*screen initialize*/
//    tft->invertDisplay(false);
//    tft->setRotation(3);
//    tft->fillScreen(COLOR_GREEN);
//    tft->drawBitmap(0, 0, esp_logo, 137, 26);
//    tft->drawString("Status: Initialize camera ...", 5, 30);
//    tft->drawString("Status: Wifi Connecting ...", 5, 44);
}
#endif
