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

#if CONFIG_USE_LCD
CEspLcd* tft = NULL;
#endif

SemaphoreHandle_t camera_sem;
static const char* TAG = "ESP-CAM-LCD";

void take_camera_sem()
{
    xSemaphoreTake(camera_sem, portMAX_DELAY);
}

void give_camera_sem()
{
    xSemaphoreGive(camera_sem);
}

void app_lcd_task(void *pvParameters)
{
#if CONFIG_USE_LCD
    uint8_t i = 0;
    uint32_t time = 0;
    time = xTaskGetTickCount();
    while(1) {
        if((xTaskGetTickCount() - time) > 1000 / portTICK_RATE_MS ){
            ESP_LOGI(TAG,"camera movie %d  fps\n", i);
            time = xTaskGetTickCount();
            i = 0;
        }
        xSemaphoreTake(camera_sem, portMAX_DELAY);
        tft->drawBitmapafterconvert(0, 0, camera_get_fb(), camera_get_fb_width(), camera_get_fb_height());
        xSemaphoreGive(camera_sem);
        i++;
    }
#endif
}

void app_lcd_init()
{
#if CONFIG_USE_LCD
    lcd_conf_t lcd_pins = {
        .lcd_model = LCD_MOD_ST7789,  //LCD_MOD_ILI9341,//LCD_MOD_ST7789,
        .pin_num_miso = CONFIG_HW_LCD_MISO_GPIO,
        .pin_num_mosi = CONFIG_HW_LCD_MOSI_GPIO,
        .pin_num_clk = CONFIG_HW_LCD_CLK_GPIO,
        .pin_num_cs = CONFIG_HW_LCD_CS_GPIO,
        .pin_num_dc = CONFIG_HW_LCD_DC_GPIO,
        .pin_num_rst = CONFIG_HW_LCD_RESET_GPIO,
        .pin_num_bckl = CONFIG_HW_LCD_BL_GPIO,
        .clk_freq = 20 * 1000 * 1000,
        .rst_active_level = 0,
        .bckl_active_level = 0,
        .spi_host = HSPI_HOST,};

    /*Initialize SPI Handler*/
    if (tft == NULL) {
        tft = new CEspLcd(&lcd_pins);
        camera_sem = xSemaphoreCreateBinary();
        xSemaphoreGive(camera_sem);     //after initialize semaphoreGive is NULL
    }

    /*screen initialize*/
    tft->invertDisplay(false);
    tft->setRotation(3);
    tft->fillScreen(COLOR_GREEN);
    tft->drawBitmap(0, 0, esp_logo, 137, 26);
#endif
}
