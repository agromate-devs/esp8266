#include "smartconfig.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"
#include "keystore.h"
#include "wifi.h"
#include "dht.h"
#include "delay.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "secrets.h"
#include "mqtt_client.h"
#include "json.h"
#include "uuid.h"

#define TAG "MQTT"
#define SUB "esp8266/sub"
#define PUB "esp8266/pub"
#define DEBUG 0
#define DHT_GPIO 5 // D1 pin

static int subscribed = 0;
static esp_mqtt_client_handle_t client;
static char *uuid = "";

void temperature_task(void *arg)
{
    int *temp_history_hour = malloc(sizeof(int) * 60);
    int *humidity_history_hour = malloc(sizeof(int) * 60);
    if(temp_history_hour == NULL || humidity_history_hour == NULL){
        ESP_LOGE("dht11", "Error allocating memory. Free memory %d", esp_get_free_heap_size());
    }

    ESP_ERROR_CHECK(dht_init(DHT_GPIO, false));
    DELAY(2000);
    int16_t humidity = 0;
    int16_t temperature = 0;
    int msg_id = 0;
    int current_minute = 0;
    while (1)
    {
        if (dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
        if(subscribed){
            if(current_minute == 59) {  // 60 finish to 59 since we start from 0
                int *media_temp = 0;
                int *media_hum = 0;
                for (size_t i = 0; i < 60; i++) 
                {
                    *media_temp += temp_history_hour[i];
                    *media_hum += humidity_history_hour[i];
                }
                *media_temp = *media_temp / 60;
                *media_hum = *media_hum / 60;
                memset(temp_history_hour, 0, sizeof(int) * 60); // Cleanup array from old misurations
                memset(humidity_history_hour, 0, sizeof(int) * 60);
                char *message = create_dht_json(*media_temp, *media_hum, uuid);
                msg_id = esp_mqtt_client_publish(client, PUB, message, 0, 0, 0);
                free(media_hum);
                free(media_temp);
                current_minute = 0;
            }
            temp_history_hour[current_minute] = temperature;
            humidity_history_hour[current_minute] = humidity;
            current_minute++;
        }
        } else {
            ESP_LOGE("DHT11", "Fail to get dht temperature data\n");
        }
        DELAY(60000);   // Delay for 1 minute
    }
    vTaskDelete(NULL);
}
int is_already_registered() {
    char *ssid = read_key("ssid", 32);
    char *password = read_key("password", 64);
    if (ssid == NULL || password == NULL) {
        return 0;
    }
    free(ssid);
    free(password);
    return 1;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = 0;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            msg_id = esp_mqtt_client_subscribe(client, SUB, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            subscribed = 0;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            subscribed = 1;
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            subscribed = 0;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_URL,
        .event_handle = mqtt_event_handler,
        .client_cert_pem = client_cert,
        .client_key_pem = privkey,
        .client_id = "ESP8266_Wemos",
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    if(is_already_registered()){
        char *ssid = read_key("ssid", 32);
        char *password = read_key("password", 64);
        wifi_init_sta(ssid, password);
        // We can destroy the SSID and password now that are stored in wifi_config and save some bytes.
        free(ssid);
        free(password);
    }else {
        initialise_smartconfig();
    }
    if(uuid_exists() == 0) {
        char *uuid = generate_uuid();
        write_key("uuid", uuid);
        free(uuid);
    }else {
        uuid = malloc(sizeof(char)* UUID_LEN);
        uuid = read_key("uuid", UUID_LEN);
        ESP_LOGI("UUID", "UUID: %s", uuid);
    }
    mqtt_app_start();
    xTaskCreate(temperature_task, "temperature task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}