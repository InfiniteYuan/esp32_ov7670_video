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
const static char http_html_hdr[] = "Content-type: text/html\r\n\r\n";

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

void Base64_Code(struct netconn *conn, unsigned char* chsrc, uint32_t len, bool head)
{
    const char *base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char char_array_3[3], char_array_4[4];
    int i = 0, j = 0, k = 0;
    unsigned char chdes1[65];
    unsigned char *chdes = chdes1;
    bool flag = true;
    uint32_t l = 0;

    while(len--)
    {
        if(head){
            char_array_3[i++] = chsrc[l++];
        }
        else{
            if(flag){
                char_array_3[i++] = chsrc[l+1];
                flag = false;
            } else {
                char_array_3[i++] = chsrc[l];
                l+=2;
                flag = true;
            }
        }
        if(3 == i)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for(i = 0; i < 4; i++){
                chdes[k++] = base64_chars[char_array_4[i]];
            }

            i = 0;
            if(k==60){
                netconn_write(conn, chdes, k, NETCONN_COPY);
                k = 0;
            }
        }
    }
    if(i)
    {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for(j = 0; j < (i+1); j++){
            chdes[k++] = base64_chars[char_array_4[j]];
        }

        while((3 > i++)){
            chdes[k++] = '=';
        }
    }
    netconn_write(conn, chdes, k, NETCONN_COPY);
    return;
}

void http_server_netconn_serve(struct netconn *conn, uint8_t frame_num)
{
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    err_t err;
    err = netconn_recv(conn, &inbuf);
    if (err == ERR_OK) {
        netbuf_data(inbuf, (void**) &buf, &buflen);
//        ESP_LOGI(TAG, "netbuf_data stop :%d, buflen: %d, buf:\n%s\n", xTaskGetTickCount(), buflen, buf);
        if (buflen >= 5 && buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'T'
                && buf[3] == ' ' && buf[4] == '/') {
            netconn_write(conn, http_hdr, sizeof(http_hdr) - 1, NETCONN_NOCOPY);
            if (memcmp(&buf[5], "bmp", 3) == 0) {
                netconn_write(conn, http_bitmap_hdr,
                        sizeof(http_bitmap_hdr) - 1, NETCONN_NOCOPY);
                char *bmp = bmp_create_header565(camera_get_fb_width(),
                        camera_get_fb_height());
                err = netconn_write(conn, bmp, sizeof(bitmap565), NETCONN_COPY);
                free(bmp);
                if ((CAMERA_PIXEL_FORMAT == CAMERA_PF_RGB565)
                        || (CAMERA_PIXEL_FORMAT == CAMERA_PF_YUV422)) {
                    uint8_t s_line[camera_get_fb_width() * 2]; //camera_get_fb_width()
                    uint32_t *fbl;
                    uint32_t *currFbPtr = camera_get_fb(frame_num);
                    for (int i = 0; i < camera_get_fb_height(); i++) { //camera_get_fb_height()
                        fbl = (uint32_t *) &currFbPtr[(i * camera_get_fb_width()) / 2]; //(i*(320*2)/4); // 4 bytes for each 2 pixel / 2 byte read..
                        convert_fb32bit_line_to_bmp565(fbl, s_line, CAMERA_PIXEL_FORMAT);
                        err = netconn_write(conn, s_line, camera_get_fb_width() * 2, NETCONN_COPY);
                    }
                } else {
                    err = netconn_write(conn, camera_get_fb(0),
                            camera_get_data_size(), NETCONN_NOCOPY);
                }
            } else if (memcmp(&buf[5], "img", 3) == 0) {
                netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                netconn_write(conn, "<html><body><div style=\"width:320px;margin:0 auto;\"><h1 style=\"text-align:center;\">camera</h1>", sizeof("<html><body><div style=\"width:320px;margin:0 auto;\"><h1 style=\"text-align:center;\">camera</h1>")-1, NETCONN_NOCOPY);
                netconn_write(conn, "<img style=\"transform: rotate(180deg);align:center\" src=\"data:image/png;base64,", sizeof("<img style=\"transform: rotate(180deg);align:center\" src=\"data:image/png;base64,")-1, NETCONN_NOCOPY);
                char *bmp = bmp_create_header565(camera_get_fb_width(), camera_get_fb_height());
                Base64_Code(conn, (unsigned char *)bmp, sizeof(bitmap565), true);
                free(bmp);
                Base64_Code(conn, (unsigned char *)camera_get_fb(frame_num), 320*240*2, false);
                netconn_write(conn, "\" alt=\"Base64 encoded image\" width=\"320\" height=\"240\"/>", sizeof("\" alt=\"Base64 encoded image\" width=\"320\" height=\"240\"/>")-1, NETCONN_NOCOPY);
                netconn_write(conn, "</div></body></html>", sizeof("</div></body></html>")-1, NETCONN_NOCOPY);
            } else {
                char outstr[120];
                get_image_mime_info_str(outstr);
                netconn_write(conn, outstr, sizeof(outstr) - 1, NETCONN_NOCOPY);
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
        if (err == ERR_OK) { /* new conn is coming */

            // http_server_netconn_serve(newconn, queue_receive());

            netconn_delete(newconn);
        }
    } while (err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
}
