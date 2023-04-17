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

#define TAG "MQTT"
#define SUB "esp8266/sub"
#define PUB "esp8266/pub"
#define DEBUG 0
#define DHT_GPIO 5 // D1 pin

static int subscribed = 0;
static esp_mqtt_client_handle_t client;

extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");

void temperature_task(void *arg)
{
    ESP_ERROR_CHECK(dht_init(DHT_GPIO, false));
    DELAY(2000);
    int16_t humidity = 0;
    int16_t temperature = 0;
    int msg_id = 0;
    while (1)
    {
        if (dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
#ifdef DEBUG
            ESP_LOGI("DHT11", "Humidity: %d Temperature: %d\n", humidity, temperature);
#endif
        if(subscribed){
#ifdef DEBUG
            ESP_LOGI("DHT11", "Sending message");
#endif
            char *message = create_dht_json(temperature, humidity);
#ifdef DEBUG
            ESP_LOGI("DHT11", message);
#endif
            msg_id = esp_mqtt_client_publish(client, PUB, message, 0, 0, 0);
#ifdef DEBUG
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
#endif
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
#ifdef DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
#endif
            msg_id = esp_mqtt_client_subscribe(client, SUB, 0);
#ifdef DEBUG
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
#endif
            break;
        case MQTT_EVENT_DISCONNECTED:
#ifdef DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
#endif
            subscribed = 0;
            break;
        case MQTT_EVENT_SUBSCRIBED:
#ifdef DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
#endif
            subscribed = 1;
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
#ifdef DEBUG
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
#endif
            subscribed = 0;
            break;
#ifdef DEBUG
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
#endif
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
#ifdef DEBUG
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
#endif
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
#ifdef DEBUG
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
#endif
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
#ifdef DEBUG
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
#endif
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    if(is_already_registered()){
        char *ssid = read_key("ssid", 32);
        char *password = read_key("password", 64);
#ifdef DEBUG
        ESP_LOGI("main", "already registered");
#endif
        wifi_init_sta(ssid, password);

        // We can destroy the SSID and password now that are stored in wifi_config and save some bytes.
        free(ssid);
        free(password);
    }else {
#ifdef DEBUG
        ESP_LOGI("main", "not registered");
#endif
        initialise_smartconfig();
    }
    mqtt_app_start();
    xTaskCreate(temperature_task, "temperature task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}