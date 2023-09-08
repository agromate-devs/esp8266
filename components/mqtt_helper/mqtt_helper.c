#include "mqtt_client.h"
#include "secrets.h"
#include "esp_log.h"
#include "uuid.h"
#include "keystore.h"
#include <string.h>
#include "sensors.h"
#include "delay.h"
#include "wifi.h"

#define TAG "MQTT"
#define SUB "esp8266/sub"
#define PUB "esp8266/pub"
#define DHT_GPIO 5 // D1 pin

int connected = 0;
esp_mqtt_client_handle_t client;
static char *uuid = NULL;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            connected = 1;
            ESP_LOGI(TAG, "MQTT connected");
            char mqtt_topic[52] = "sensor/plants/"; // The UUID is 37 characters long plus original mqtt_topic will be approximately 52 characters long
            strcat(mqtt_topic, uuid);
            esp_mqtt_client_subscribe(client, mqtt_topic, 0);       // Listen for changes
            break;
        case MQTT_EVENT_DISCONNECTED:
            connected = 0;
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            write_key("current_plant", event->data);
            if(plant_assigned){
                vTaskDelete(temperature_task_handle);   // Destroy the old task
                init_sensors_mqtt(uuid, client);    // Reinit all
            }else {
                plant_assigned = 1;
                init_sensors_mqtt(uuid, client);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_URL,
        .event_handle = mqtt_event_handler,
        .client_cert_pem = client_cert,
        .client_key_pem = privkey,
        .client_id = "ESP8266"
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void mqtt_task(void *arg) {
    uuid = read_key("uuid", UUID_LEN);
    while(1){
        if(wifi_connected) {
            mqtt_app_start();
            init_sensors_mqtt(uuid, client);
            vTaskDelete(NULL);  // Delete the task since sensors have been initialized
        }else {
            ESP_LOGI(TAG, "Waiting for wifi connection...");
        }
        DELAY(5000);    // Wait 5 seconds before trying again
    }
}