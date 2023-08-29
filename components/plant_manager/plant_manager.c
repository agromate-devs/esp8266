/* HTTPS GET Example adapted for AgroMate
 * Adapted from the ssl_client1 example in mbedtls.
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 * Additions Copyright (C) 2016-2017 AgroMate-devs. All rights reserved. Apache 2.0 License.      
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "cJSON.h"
#include "sensor.h"
#include "../wifi/wifi.h"
#include "../mqtt_helper/mqtt_helper.h"

#define WEB_SERVER "www.bysiftg28d.execute-api.eu-central-1.amazonaws.com"
#define WEB_PORT 443
#define WEB_URL "https://bysiftg28d.execute-api.eu-central-1.amazonaws.com/plant_info_api?user_id=uid&sensor_id=UUID"

#define BUFFER_SIZE 512
#define MAX_RESPONSE_LENGHT 1024

#define DELAY(x) vTaskDelay(x / portTICK_PERIOD_MS)

TaskHandle_t plant_task_handle;

static const char *TAG = "example";

static const char *REQUEST = "GET " WEB_URL " HTTP/1.0\r\n"
                             "Host: " WEB_SERVER "\r\n"
                             "User-Agent: esp-idf/1.0 esp32\r\n"
                             "\r\n";

void read_response(struct esp_tls *tls, char *response)
{
    int len = 0, ret = 0;
    char buf[BUFFER_SIZE];
    do
    {
        len = sizeof(buf) - 1;
        bzero(buf, BUFFER_SIZE);
        ret = esp_tls_conn_read(tls, buf, len);

        if (ret == ESP_TLS_ERR_SSL_WANT_READ || ret == ESP_TLS_ERR_SSL_WANT_WRITE)
            continue;

        if (ret < 0)
        {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned -0x%x", -ret);
            ESP_LOGE(TAG, "esp_tls_conn_read  returned -0x%x", -ret);
            break;
        }

        if (ret == 0)
        {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        strncat(response, buf, len);
    } while (1);
}

char *https_get_task(void)
{
    int ret;
    esp_tls_cfg_t cfg = {};
    char *response = NULL;
    struct esp_tls *tls = esp_tls_conn_http_new(WEB_URL, &cfg);

    if (tls != NULL)
    {
        ESP_LOGI(TAG, "Connection established...");
    }
    else
    {
        ESP_LOGE(TAG, "Connection failed...");
        goto exit;
    }

    size_t written_bytes = 0;
    do
    {
        ret = esp_tls_conn_write(tls,
                                 REQUEST + written_bytes,
                                 strlen(REQUEST) - written_bytes);
        if (ret >= 0)
        {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        }
        else if (ret != ESP_TLS_ERR_SSL_WANT_READ && ret != ESP_TLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned 0x%x", ret);
            goto exit;
        }
    } while (written_bytes < strlen(REQUEST));

    ESP_LOGI(TAG, "Reading HTTP response...");
    response = malloc(MAX_RESPONSE_LENGHT * sizeof(char));
    bzero(response, MAX_RESPONSE_LENGHT);  // Cleanup string since we concatenate it with tls buffer
    read_response(tls, response);

    goto exit;

exit:
    ESP_LOGI(TAG, "Deleting TLS connection...");
    esp_tls_conn_delete(tls);
    ESP_LOGI(TAG, "FINISH");
    return response;
    // }
}

double parse_float(cJSON *json, char *key) {
    cJSON *default_humidity = cJSON_GetObjectItemCaseSensitive(json, key);
    if(cJSON_IsNumber(default_humidity)){
        ESP_LOGI(TAG, "Key %s found", key);
        return default_humidity->valuedouble;
    }else{
        ESP_LOGE(TAG, "Invalid %s value", key);
        return -1.0;
    }
}

void cleanup_response(char *response) {
    char *start = strstr(response, "{");  // Begin of JSON
    strcpy(response, start);    // Copy from start to end of JSON body in response
}

TemperatureTask parse_raw_response(char *response){
    cleanup_response(response); // Remove header of response and use only JSON body
    cJSON *json = cJSON_Parse(response);
    double default_humidity_value = parse_float(json, "default_humidity");
    double default_temperature_value = parse_float(json, "default_temperature");
    ESP_LOGI(TAG, "Default humidity: %d", (int)(default_humidity_value * 10));
    ESP_LOGI(TAG, "Default temperature: %d", (int)(default_temperature_value * 10));
    TemperatureTask limits = {
        .temperature_limit = default_temperature_value,
        .humidity_limit = default_humidity_value
    };

    return limits;
}

void plant_task(void *arg) {
    char *uuid = (char *)arg;
    ESP_LOGI(TAG, "UUID: %s", uuid);
    while(1){
        if(wifi_connected) {
            char *response = https_get_task();
            ESP_LOGI(TAG, "Response: %s", response);
            TemperatureTask limits = parse_raw_response(response);
            init_sensors_mqtt(uuid, mqtt_app_start(), &limits);
            // xTaskCreate(temperature_task, "temperature task", 2048, &limits, tskIDLE_PRIORITY, NULL);
            vTaskDelete(plant_task_handle);
        }else {
            ESP_LOGI(TAG, "Waiting for wifi connection...");
        }
        DELAY(20000);
    }
}