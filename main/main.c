// #include <stdio.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <esp_err.h>


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

#define DHT_GPIO 5 // D1 pin

void temperature_task(void *arg)
{
    ESP_ERROR_CHECK(dht_init(DHT_GPIO, false));
    DELAY(2000);
    while (1)
    {
        int16_t humidity = 0;
        int16_t temperature = 0;
        if (dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature) == ESP_OK) {
#ifdef DEBUG
            ESP_LOGI("DHT11", "Humidity: %d Temperature: %d\n", humidity, temperature);
#endif
        } else {
            ESP_LOGE("DHT11", "Fail to get dht temperature data\n");
        }
        DELAY(500);
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

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
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
    xTaskCreate(temperature_task, "temperature task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}