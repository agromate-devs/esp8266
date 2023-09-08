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
#include "../keystore/keystore.h"
#include "uuid.h"

#define TAG "json_helper"

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

TemperatureTask parse_raw_response(char *response){
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