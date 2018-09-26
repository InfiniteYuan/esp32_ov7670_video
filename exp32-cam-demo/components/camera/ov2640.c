/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * OV2640 driver.
 *
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sccb.h"
#include "ov2640.h"
#include "ov2640_regs.h"
#include "ov2640_settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
static const char* TAG = "ov2640";
#endif

static void write_regs(uint8_t slv_addr, const uint8_t (*regs)[2]){
    int i=0;
    while (regs[i][0]) {
        SCCB_Write(slv_addr, regs[i][0], regs[i][1]);
        i++;
    }
}

static int reset(sensor_t *sensor)
{
    /* Reset all registers */
    SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);
    SCCB_Write(sensor->slv_addr, COM7, COM7_SRST);

    /* delay n ms */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    write_regs(sensor->slv_addr, ov2640_settings_cif);

    return 0;
}

static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
    /* read pixel format reg */
    switch (pixformat) {
        case PIXFORMAT_RGB565:
            write_regs(sensor->slv_addr, ov2640_settings_rgb565);
            break;
        case PIXFORMAT_YUV422:
        case PIXFORMAT_GRAYSCALE:
            write_regs(sensor->slv_addr, ov2640_settings_yuv422);
            break;
        case PIXFORMAT_JPEG:
            write_regs(sensor->slv_addr, ov2640_settings_jpeg3);
            break;
        default:
            return -1;
    }

    /* delay n ms */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return 0;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{
    int ret=0;
    uint16_t w = resolution[framesize][0];
    uint16_t h = resolution[framesize][1];
    const uint8_t (*regs)[2];

    if (framesize <= FRAMESIZE_CIF) {
        regs = ov2640_settings_to_cif;
    } else if (framesize <= FRAMESIZE_SVGA) {
        regs = ov2640_settings_to_svga;
    } else {
        regs = ov2640_settings_to_uxga;
    }
    
    /* Disable DSP */

    //ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);
    //ret |= SCCB_Write(sensor->slv_addr, R_BYPASS, R_BYPASS_DSP_BYPAS);

    /* Write DSP input regsiters */
    write_regs(sensor->slv_addr, regs);

    /* Write output width */
    ret |= SCCB_Write(sensor->slv_addr, ZMOW, (w>>2)&0xFF); // OUTW[7:0] (real/4)
    ret |= SCCB_Write(sensor->slv_addr, ZMOH, (h>>2)&0xFF); // OUTH[7:0] (real/4)
    ret |= SCCB_Write(sensor->slv_addr, ZMHH, ((h>>8)&0x04)|((w>>10)&0x03)); // OUTH[8]/OUTW[9:8]

    /* Reset DSP */
    ret |= SCCB_Write(sensor->slv_addr, RESET, 0x00);

    /* Enable DSP */
    //ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);
    //ret |= SCCB_Write(sensor->slv_addr, R_BYPASS, R_BYPASS_DSP_EN);
    /* delay n ms */
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return ret;
}

static int set_framerate(sensor_t *sensor, framerate_t framerate)
{
    return 0;
}

static int set_contrast(sensor_t *sensor, int level)
{
    int ret=0;

    level += (NUM_CONTRAST_LEVELS / 2 + 1);
    if (level < 0 || level > NUM_CONTRAST_LEVELS) {
        return -1;
    }

    /* Switch to DSP register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    /* Write contrast registers */
    for (int i=0; i<sizeof(contrast_regs[0])/sizeof(contrast_regs[0][0]); i++) {
        ret |= SCCB_Write(sensor->slv_addr, contrast_regs[0][i], contrast_regs[level][i]);
    }

    return ret;
}

static int set_brightness(sensor_t *sensor, int level)
{
    int ret=0;

    level += (NUM_BRIGHTNESS_LEVELS / 2 + 1);
    if (level < 0 || level > NUM_BRIGHTNESS_LEVELS) {
        return -1;
    }

    /* Switch to DSP register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    /* Write brightness registers */
    for (int i=0; i<sizeof(brightness_regs[0])/sizeof(brightness_regs[0][0]); i++) {
        ret |= SCCB_Write(sensor->slv_addr, brightness_regs[0][i], brightness_regs[level][i]);
    }

    return ret;
}

static int set_saturation(sensor_t *sensor, int level)
{
    int ret=0;

    level += (NUM_SATURATION_LEVELS / 2 + 1);
    if (level < 0 || level > NUM_SATURATION_LEVELS) {
        return -1;
    }

    /* Switch to DSP register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    /* Write contrast registers */
    for (int i=0; i<sizeof(saturation_regs[0])/sizeof(saturation_regs[0][0]); i++) {
        ret |= SCCB_Write(sensor->slv_addr, saturation_regs[0][i], saturation_regs[level][i]);
    }

    return ret;
}

static int set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{
    int ret =0;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    /* Write gain ceiling register */
    ret |= SCCB_Write(sensor->slv_addr, COM9, COM9_AGC_SET(gainceiling));

    return ret;
}

static int set_quality(sensor_t *sensor, int qs)
{
    int ret=0;

    /* Switch to DSP register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    /* Write QS register */
    ret |= SCCB_Write(sensor->slv_addr, QS, qs);

    return ret;
}

static int set_colorbar(sensor_t *sensor, int enable)
{
    int ret=0;
    uint8_t reg;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    /* Update COM7 */
    reg = SCCB_Read(sensor->slv_addr, COM7);

    if (enable) {
        reg |= COM7_COLOR_BAR;
    } else {
        reg &= ~COM7_COLOR_BAR;
    }

    ret |= SCCB_Write(sensor->slv_addr, COM7, reg);
    return ret;
}

static int set_whitebal(sensor_t *sensor, int enable)
{
    int ret=0;
    uint8_t reg;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_DSP);

    /* Update CTRL1 */
    reg = SCCB_Read(sensor->slv_addr, CTRL1);

    if (enable) {
        reg |= CTRL1_AWB;
    } else {
        reg &= ~CTRL1_AWB;
    }

    ret |= SCCB_Write(sensor->slv_addr, CTRL1, reg);
    return ret;
}

static int set_gain_ctrl(sensor_t *sensor, int enable)
{
    int ret=0;
    uint8_t reg;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    /* Update COM8 */
    reg = SCCB_Read(sensor->slv_addr, COM8);

    if (enable) {
        reg |= COM8_AGC_EN;
    } else {
        reg &= ~COM8_AGC_EN;
    }

    ret |= SCCB_Write(sensor->slv_addr, COM8, reg);
    return ret;
}

static int set_exposure_ctrl(sensor_t *sensor, int enable)
{
    int ret=0;
    uint8_t reg;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    /* Update COM8 */
    reg = SCCB_Read(sensor->slv_addr, COM8);

    if (enable) {
        reg |= COM8_AEC_EN;
    } else {
        reg &= ~COM8_AEC_EN;
    }

    ret |= SCCB_Write(sensor->slv_addr, COM8, reg);
    return ret;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
    int ret=0;
    uint8_t reg;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    /* Update REG04 */
    reg = SCCB_Read(sensor->slv_addr, REG04);

    if (enable) {
        reg |= REG04_HFLIP_IMG;
    } else {
        reg &= ~REG04_HFLIP_IMG;
    }

    ret |= SCCB_Write(sensor->slv_addr, REG04, reg);
    return ret;
}

static int set_vflip(sensor_t *sensor, int enable)
{
    int ret=0;
    uint8_t reg;

    /* Switch to SENSOR register bank */
    ret |= SCCB_Write(sensor->slv_addr, BANK_SEL, BANK_SEL_SENSOR);

    /* Update REG04 */
    reg = SCCB_Read(sensor->slv_addr, REG04);

    if (enable) {
        reg |= REG04_VFLIP_IMG;
    } else {
        reg &= ~REG04_VFLIP_IMG;
    }

    ret |= SCCB_Write(sensor->slv_addr, REG04, reg);
    return ret;
}

int ov2640_init(sensor_t *sensor)
{
    /* set function pointers */
    sensor->reset = reset;
    sensor->set_pixformat = set_pixformat;
    sensor->set_framesize = set_framesize;
    sensor->set_framerate = set_framerate;
    sensor->set_contrast  = set_contrast;
    sensor->set_brightness= set_brightness;
    sensor->set_saturation= set_saturation;
    sensor->set_gainceiling = set_gainceiling;
    sensor->set_quality = set_quality;
    sensor->set_colorbar = set_colorbar;
    sensor->set_gain_ctrl = set_gain_ctrl;
    sensor->set_exposure_ctrl = set_exposure_ctrl;
    sensor->set_whitebal = set_whitebal;
    sensor->set_hmirror = set_hmirror;
    sensor->set_vflip = set_vflip;

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 0);

    return 0;
}
