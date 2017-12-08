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

static const char* TAG = "ESP-CAM-HTTP-SERVER";

// camera code
const static char http_hdr[] = "HTTP/1.1 200 OK\r\n";
const static char http_bitmap_hdr[] = "Content-type: image/bitmap\r\n\r\n";

inline uint8_t unpack(int byteNumber, uint32_t value)
{
    return (value >> (byteNumber * 8));
}

void convert_fb32bit_line_to_bmp565(uint32_t *srcline, uint8_t *destline,
        const camera_pixelformat_t format)
{
    uint16_t pixel565 = 0;
    uint16_t pixel565_2 = 0;
    uint32_t long2px = 0;
    uint16_t *sptr;
    uint16_t conver_times;
    uint16_t current_src_pos = 0, current_dest_pos = 0;

    switch (CAMERA_FRAME_SIZE) {
        case CAMERA_FS_SVGA:
            conver_times = 800;
            break;
        case CAMERA_FS_VGA:
            conver_times = 640;
            break;
        case CAMERA_FS_QVGA:
            conver_times = 320;
            break;
        case CAMERA_FS_QCIF:
            conver_times = 176;
            break;
        case CAMERA_FS_HQVGA:
            conver_times = 240;
            break;
        case CAMERA_FS_QQVGA:
            conver_times = 160;
            break;
        default:
            printf("framesize not init\n");
            break;
    }

    for (int current_pixel_pos = 0; current_pixel_pos < conver_times;
            current_pixel_pos += 2) {
        current_src_pos = current_pixel_pos / 2;
        long2px = srcline[current_src_pos];

        pixel565 = (unpack(2, long2px) << 8) | unpack(3, long2px);
        pixel565_2 = (unpack(0, long2px) << 8) | unpack(1, long2px);

        sptr = (uint16_t *) &destline[current_dest_pos];
        *sptr = pixel565;
        sptr = (uint16_t *) &destline[current_dest_pos + 2];
        *sptr = pixel565_2;
        current_dest_pos += 4;

    }
}

void http_server_netconn_serve(struct netconn *conn)
{
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    err_t err;
    err = netconn_recv(conn, &inbuf);
    if (err == ERR_OK) {
        netbuf_data(inbuf, (void**) &buf, &buflen);
        ESP_LOGI(TAG, "netbuf_data stop :%d, buflen: %d, buf:\n%s\n", xTaskGetTickCount(), buflen, buf);
        if (buflen >= 5 && buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T'
                && buf[3] == ' ' && buf[4] == '/') {
            ESP_LOGD(TAG, "Start Image Sending...");
            netconn_write(conn, http_hdr, sizeof(http_hdr) - 1, NETCONN_NOCOPY);
            netconn_write(conn, http_bitmap_hdr, sizeof(http_bitmap_hdr) - 1, NETCONN_NOCOPY);
            if (memcmp(&buf[5], "bmp", 3) == 0) {
                char *bmp = bmp_create_header565(camera_get_fb_width(),
                        camera_get_fb_height());
                err = netconn_write(conn, bmp, sizeof(bitmap565), NETCONN_COPY);
                free(bmp);
            } else {
                char outstr[120];
                get_image_mime_info_str(outstr);
                netconn_write(conn, outstr, sizeof(outstr) - 1, NETCONN_NOCOPY);
            }

            if ((CAMERA_PIXEL_FORMAT == CAMERA_PF_RGB565)
                    || (CAMERA_PIXEL_FORMAT == CAMERA_PF_YUV422)) {
                uint8_t s_line[camera_get_fb_width() * 2]; //camera_get_fb_width()
                uint32_t *fbl;
                uint32_t *currFbPtr = camera_get_fb();
                for (int i = 0; i < camera_get_fb_height(); i++) { //camera_get_fb_height()
                    fbl = (uint32_t *) &currFbPtr[(i * camera_get_fb_width()) / 2]; //(i*(320*2)/4); // 4 bytes for each 2 pixel / 2 byte read..
                    convert_fb32bit_line_to_bmp565(fbl, s_line, CAMERA_PIXEL_FORMAT);
                    err = netconn_write(conn, s_line, camera_get_fb_width() * 2, NETCONN_COPY);
                }
                ESP_LOGD(TAG, "Image sending Done ...");
            } else {
                err = netconn_write(conn, camera_get_fb(), camera_get_data_size(), NETCONN_NOCOPY);
            }

        }  // end GET request:
    }
    netconn_close(conn); /* Close the connection (server closes in HTTP) */
    netbuf_delete(inbuf);/* Delete the buffer (netconn_recv gives us ownership,so we have to make sure to deallocate the buffer) */
}

void http_server_task(void *pvParameters)
{
    uint8_t i = 0;
    struct netconn *conn, *newconn;
    err_t err, ert;
    conn = netconn_new(NETCONN_TCP); /* creat TCP connector */
    netconn_bind(conn, NULL, 80); /* bind HTTP port */
    netconn_listen(conn); /* server listen connect */
    do {
        ESP_LOGI(TAG, "netconn_accept start :%d\n", xTaskGetTickCount());
        err = netconn_accept(conn, &newconn);
        ESP_LOGI(TAG, "netconn_accept stop :%d\n", xTaskGetTickCount());
        if (err == ERR_OK) { /* new conn is coming */
            printf("camera_run start :%d\n", xTaskGetTickCount());

            take_camera_sem();
            ESP_LOGI(TAG, "http_server->xSemaphoreTake:::%d\n", i);
            ert = camera_run();
            printf("%s\n",
                    ert == ERR_OK ? "camer run success" : "camera run failed");

            http_server_netconn_serve(newconn);

            ESP_LOGI(TAG, "http_server->xSemaphoreGive:::%d\n", i++);
            give_camera_sem();

            netconn_delete(newconn);
        }
    } while (err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
}
