#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "smartconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "keystore.h"
#include "wifi.h"
#include "dht.h"
#include "delay.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "json.h"
#include "uuid.h"
#include "mqtt_helper.h"
#include "sensors.h"

#define TAG "main"
#define DEBUG 0

static char *uuid = "";

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    if(is_already_registered()){
        init_wifi_connection();
    }else {
        initialise_smartconfig();
    }

    if(uuid_exists() == 0) {
        char *uuid = generate_uuid();
        write_key("uuid", uuid);
    }else {
        uuid = malloc(sizeof(char)* UUID_LEN);
        uuid = read_key("uuid", UUID_LEN);
    }

    // SensorTask *sensors_info = malloc(sizeof(SensorTask));
    // strcpy(temp->uuid, uuid);
    // temp->client = mqtt_app_start();
    // // strcpy(uuid_sensor, uuid);
    // // client_sensor = mqtt_app_start();

}