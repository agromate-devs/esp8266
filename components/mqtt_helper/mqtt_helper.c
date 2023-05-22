#include "mqtt_client.h"
#include "secrets.h"

#define TAG "MQTT"
#define SUB "esp8266/sub"
#define PUB "esp8266/pub"
#define DHT_GPIO 5 // D1 pin

int subscribed = 0;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            esp_mqtt_client_subscribe(client, SUB, 0);
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
            break;
        default:
            break;
    }
    
    return ESP_OK;
}

esp_mqtt_client_handle_t mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_URL,
        .event_handle = mqtt_event_handler,
        .client_cert_pem = client_cert,
        .client_key_pem = privkey,
        .client_id = "ESP8266_Wemos",
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    return client;
}