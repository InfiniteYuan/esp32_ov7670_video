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
#include "iot_tcp.h"
#include "iot_udp.h"
#include "iot_wifi_conn.h"
#include "app_camera.h"
#include "nvs_flash.h"
#include "esp_wifi_internal.h"
#include "string.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"

#define SERVER_PORT             8080
#define SERVER_MAX_CONNECTION   20

static const char* TAG_SRV = "TCP_SRV";
static const char* TAG_UDP_SRV = "UDP_SRV";
static EventGroupHandle_t wifi_event_group;
volatile static uint32_t * * currFbPtr __attribute__ ((aligned(4))) = NULL;

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
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

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

void initialise_buffer()
{
    currFbPtr= (volatile uint32_t **)malloc(sizeof(uint32_t *) * CAMERA_CACHE_NUM);

    ESP_LOGI(TAG_SRV, "get free size of 32BIT heap : %d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT));

    for(int i = 0; i < CAMERA_CACHE_NUM; i++){
        currFbPtr[i] = (volatile uint32_t *) heap_caps_malloc(320 * 240 * 2, MALLOC_CAP_32BIT);
        ESP_LOGI(TAG_SRV, "framebuffer address is:%p\n", currFbPtr[i]);
    }
}

extern "C" void tcp_client_obj_test()
{
   CTcpConn client;
   const char* data = "test1234567";
   uint8_t recv_buf[100];

   tcpip_adapter_ip_info_t ipconfig;
   tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipconfig);

   ESP_LOGI(TAG_SRV, "TCP connect...");
   ESP_LOGI(TAG_SRV, "connect to: %d.%d.%d.%d(%d)", ((uint8_t*)&ipconfig.ip.addr)[0], ((uint8_t*)&ipconfig.ip.addr)[1],
                                                     ((uint8_t*)&ipconfig.ip.addr)[2], ((uint8_t*)&ipconfig.ip.addr)[3], SERVER_PORT);

   if (client.Connect(ipconfig.ip.addr, SERVER_PORT) < 0) {
       ESP_LOGI(TAG_SRV, "fail to connect...");
       vTaskDelay(5000 / portTICK_PERIOD_MS);
   }

   while(1) {
       if (client.Write((const uint8_t*)data, strlen(data)) < 0) {
           vTaskDelay(5000 / portTICK_PERIOD_MS);
       }
   }
}

void tcp_server_handle(void* arg)
{
    static uint8_t i = 0;
    uint8_t* data;
    uint16_t data1;
    CTcpConn *conn = (CTcpConn*) arg;
    while (1) {
        if (conn) {
            data = (uint8_t *)currFbPtr[i % CAMERA_CACHE_NUM];
            for(int j = 0;j < 150; j++){
                if((data1 = conn->Read(data, 1024, 10)) != 1024){
                    ESP_LOGI(TAG_SRV, "recv %d num: %d", j, data1);
                    data += data1;
                    data1 = conn->Read(data, 1024-data1, 10);
                    data += data1;
                    ESP_LOGI(TAG_SRV, "*****recv %d num: %d", j, data1);
                } else {
                    ESP_LOGI(TAG_SRV, "recv %d num: %d", j, data1);
                    data += 1024;
                }
            }
            i = i % CAMERA_CACHE_NUM;
            queue_send((uint16_t *)currFbPtr[i++]);
        }
    }
}

void tcp_server_task(void *pvParameters)
{
   CTcpServer server;
   server.Listen(SERVER_PORT, SERVER_MAX_CONNECTION);   

   while (1) {
       CTcpConn* conn = server.Accept();
       ESP_LOGI(TAG_SRV, "CTcpConn connected...");
       xTaskCreate(tcp_server_handle, "tcp_server_handle", 2048, (void* )conn, 6, NULL);
   }
}

void udp_server_task(void* pvParameters)
{
    CUdpConn server;
    static uint8_t i = 0;
    int recv_num = 0;
    bool recv_ask_flag = false;
    uint8_t recv_buf[5];

    server.Bind(7777);
    server.SetTimeout(1);

    uint8_t* recv_data;
    struct sockaddr_in remoteAddr;
    size_t nAddrLen = sizeof(remoteAddr);
    while (1) {
        /*********start*********/
        recv_ask_flag = false;
        while(!recv_ask_flag){
            recv_num = server.RecvFrom(recv_buf, sizeof(recv_buf), (struct sockaddr*) &remoteAddr, &nAddrLen);
            if(recv_num < 0){ //send_again
                // ESP_LOGI(TAG_UDP_SRV, "start error");
                server.SendTo("11111", strlen("11111"), 0, (struct sockaddr*) &remoteAddr);
            } else if(recv_buf[0] == 's'){
                // ESP_LOGI(TAG_UDP_SRV, "start ok");
                start_ask:
                recv_ask_flag = true;
                server.SendTo("start", strlen("start"), 0, (struct sockaddr*) &remoteAddr);
            }
        }
        /*********start*********/
        recv_data = (uint8_t *)currFbPtr[i % CAMERA_CACHE_NUM];
        for(int j = 0;j < 150; j++){
            recv_ask_flag = false;
            while(!recv_ask_flag){
                recv_num = server.RecvFrom(recv_data, 1024, (struct sockaddr*) &remoteAddr, &nAddrLen);
                // ESP_LOGI(TAG_UDP_SRV, "j:%d,RecvFrom:%d", j, recv_num);
                if (recv_num < 0) { //send_again
                    server.SendTo("11111", strlen("11111"), 0, (struct sockaddr*) &remoteAddr);
                } else if(recv_num == 1024){ //ack
                    recv_data += 1024;
                    recv_ask_flag = true;
                    server.SendTo("22222", strlen("22222"), 0, (struct sockaddr*) &remoteAddr);
                } else if(recv_data[0] == 's') {
                    goto start_ask;
                }
            }
        }
        i = i % CAMERA_CACHE_NUM;
        queue_send((uint16_t *)currFbPtr[i++]);
    }
    vTaskDelete(NULL);
}