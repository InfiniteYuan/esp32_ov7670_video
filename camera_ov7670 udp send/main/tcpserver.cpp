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
#include "lwip/inet.h"
#include "lwip/sockets.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/def.h"

#include "iot_tcp.h"
#include "iot_udp.h"
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
static const char* TAG_UDP_SRV = "UDP_SRV";
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

//    if (client.Connect("192.168.4.1", 8080) < 0) {
    if (client.Connect("192.168.0.5", 7777) < 0) {
        ESP_LOGI(TAG_SRV, "fail to connect...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    uint32_t time = 0;
    uint32_t time1 = 0;
    time = xTaskGetTickCount();
    uint32_t i = 0;
    while (1) {
        num = queue_receive();
        data = (uint8_t*) camera_get_fb(num);
        if((xTaskGetTickCount() - time) > 1000 / portTICK_RATE_MS ) {
             ESP_LOGI(TAG_UDP_SRV,"sendto %d  fps", i);
             time = xTaskGetTickCount();
             i = 0;
         }
        for (int j = 0; j < 150; j++) {
//            ESP_LOGI(TAG_SRV, "send %d data:%d", j,
//                    client.Write((const uint8_t * ) data, 1024));
//            vTaskDelay(100 / portTICK_PERIOD_MS);
            ESP_LOGI(TAG_SRV, "start send");
            client.Write((const uint8_t * ) data, 1024);
            ESP_LOGI(TAG_SRV, "stop send");
            data += 1024;
            i++;
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

typedef struct
{
    uint8_t pack_num;
    uint8_t* data_point;
} data_pack;

void udp_client_task(void *pvParameters)
{
//    CUdpConn client;
    int socketid;
    uint8_t * data;
    uint8_t * data1;
    uint8_t recv_num = 0;
    uint8_t send_num = 0;
    data_pack pack;

    uint8_t recv_buf[5];
    bool recv_ask_flag = false;

    socketid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 1;
    receiving_timeout.tv_usec = 0;
    setsockopt(socketid, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout));

    struct sockaddr_in remoteAddr;
    size_t nAddrLen = sizeof(remoteAddr);
    while (1) {
        data = (uint8_t*) camera_get_fb(queue_receive());
        data1 = data;
        /***************start***************/
        sockaddr_in sin;
        sin.sin_family = AF_INET;
        sin.sin_port = htons(7777);
        sin.sin_addr.s_addr = inet_addr("192.168.4.1");
        int len = sizeof(sin);
        start: int ret = sendto(socketid, "start", 5, 0, (sockaddr *) &sin,
                len);

        recv_ask_flag = false;
        while (!recv_ask_flag) {
//            recv_num = client.RecvFrom(recv_buf, sizeof(recv_buf), (struct sockaddr*) &remoteAddr, &nAddrLen);
            recv_num = recvfrom(socketid, recv_buf, sizeof(recv_buf), 0,
                    (struct sockaddr*) &remoteAddr, &nAddrLen);

//            ESP_LOGI(TAG_UDP_SRV, "start wait:%d,%s", recv_num, recv_buf);
            if (recv_num < 0) {
//                ESP_LOGI(TAG_UDP_SRV, "start error");
                continue;
            } else if (recv_buf[0] == 's') {
                ESP_LOGI(TAG_UDP_SRV, "start ok");
                recv_ask_flag = true;
            } else if (recv_buf[0] == '3') {
                sendto(socketid, "start", 5, 0, (sockaddr *) &sin, len);
            }
        }
        /***************start***************/
//        for (int i = 0; i < 150; i++) {
//            pack.pack_num = i;
//            pack.data_point = data1;
//            sendto(socketid, (uint8_t *)pack, 1025, 0, (sockaddr * )&sin, len);
//        }
        /*********** test ************/
//        int j = 0;
//        uint32_t time = 0;
//        time = xTaskGetTickCount();
//        ESP_LOGI(TAG_UDP_SRV, "start send");
//        while(1) {
//            if((xTaskGetTickCount() - time) > 1000 / portTICK_RATE_MS ){
//                ESP_LOGI(TAG_UDP_SRV,"app_camera_task movie %d fps", j);
//                time = xTaskGetTickCount();
//                j = 0;
//            }
//            data1 = data;
//            for (int i = 0; i < 150; i++) {
//                sendto(socketid, data1, 1024, 0, (sockaddr * )&sin, len);
//                data1 += 1024;
//                j++;
//            }
//        }
        /*********** test ************/

        for (int i = 0; i < 150; i++) {
            sendto(socketid, data, 1024, 0, (sockaddr *) &sin, len);
            recv_ask_flag = false;
            while (!recv_ask_flag) {
                recv_num = recvfrom(socketid, recv_buf, sizeof(recv_buf), 0,
                        (struct sockaddr*) &remoteAddr, &nAddrLen);
                if (recv_num < 0) {
                    continue;
                } else {
                    if (recv_buf[0] == '2') { //ack
                        data += 1024;
                        recv_ask_flag = true;
                    } else if (recv_buf[0] == '1') { //send again
                        sendto(socketid, data, 1024, 0, (sockaddr *) &sin, len);
                    } else if (recv_buf[0] == '3') {
                        goto start;
                    }
                }
            }
        }
    }
}

void udp_client_send_pack_task(void *pvParameters)
{
//    CUdpConn client;
    int socketid;
    uint8_t * data;
    uint8_t * data1;
    uint8_t recv_num = 0;
    uint16_t send_num = 0;
    uint8_t frame_num = 0;
    uint8_t* pack = (uint8_t*) pvParameters;

    uint8_t recv_buf[5];
    bool recv_ask_flag = false;
    bool send_comp = false;

    socketid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 1;
    receiving_timeout.tv_usec = 0;
    setsockopt(socketid, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
            sizeof(receiving_timeout));

    struct sockaddr_in remoteAddr;
    size_t nAddrLen = sizeof(remoteAddr);

    sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(7777);
    remote_addr.sin_addr.s_addr = inet_addr("192.168.4.1");
//    sin.sin_addr.s_addr = inet_addr("192.168.0.5");
    int len = sizeof(remote_addr);

    uint32_t time = 0;
    uint32_t time1 = 0;
    time = xTaskGetTickCount();
    uint32_t j = 0;

    while (1) {
        frame_num = queue_receive();
        if((xTaskGetTickCount() - time) > 1000 / portTICK_RATE_MS ) {
             ESP_LOGI(TAG_UDP_SRV,"sendto %d  fps", j);
             time = xTaskGetTickCount();
             j = 0;
         }
//        ESP_LOGI(TAG_UDP_SRV, "queue_receive %d ms", (time1-time)*portTICK_RATE_MS);
        data = (uint8_t*) camera_get_fb(frame_num);
        data1 = data;
        for (int i = 0; i < 150; i++) {
            pack[0] = i>>8;
            pack[1] = i&0xff;
            memcpy(pack + 2, data1 + i * 1024, 1024);
            send_num = sendto(socketid, pack, 1026, 0, (sockaddr *) &remote_addr, len);
//            ESP_LOGI(TAG_UDP_SRV, "send num %d", send_num);
//            sendto(socketid, data1 + i * 10240, 10240, 0, (sockaddr *) &sin, len);

        }
        j++;
    }
}
