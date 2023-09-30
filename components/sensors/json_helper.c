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
#include "sensors.h"
#include "../wifi/wifi.h"
#include "../mqtt_helper/mqtt_helper.h"
#include "../keystore/keystore.h"
#include "uuid.h"

#define TAG "json_helper"

double parse_float(cJSON *json, char *key) {
    cJSON *default_humidity = cJSON_GetObjectItemCaseSensitive(json, key);
    if(cJSON_IsNumber(default_humidity)){
        ESP_LOGI(TAG, "Key %s found with value: %f", key, default_humidity->valuedouble);
        return default_humidity->valuedouble;
    }else{
        ESP_LOGE(TAG, "Invalid %s value, maybe null?", key);
        return -1.0;
    }
}

int parse_bool(cJSON *json, char *key) {
    cJSON *value = cJSON_GetObjectItemCaseSensitive(json, key);
    if(cJSON_IsBool(value)){
        ESP_LOGI(TAG, "Key %s found", key);
        return value->valueint;
    }else {
        ESP_LOGW(TAG, "Invalid %s value, maybe null?", key);
        return 0;
    }
}

char *parse_string(cJSON *json, char *key) {
    cJSON *value = cJSON_GetObjectItemCaseSensitive(json, key);
    if(cJSON_IsString(value)){
        ESP_LOGI(TAG, "Key %s found with value: %s", key, value->valuestring);
        return value->valuestring;
    }else {
        ESP_LOGW(TAG, "Invalid %s value, maybe null?", key);
        return NULL;
    }
}

Plant parse_raw_response(char *response){
    cJSON *json = cJSON_Parse(response);

    float temp_limit = parse_float(json, "default_humidity");
    float humidity_limit = parse_float(json, "default_humidity");
    int temperature_notification = parse_bool(json, "notify_wrong_temperature");
    int humidity_notification = parse_bool(json, "notify_wrong_humidity");
    char *user_id = parse_string(json, "user_id");

    Plant limits = {
        .temperature_limit = temp_limit ,
        .humidity_limit = humidity_limit,
        .humidity_notification = humidity_notification,
        .temperature_notification = temperature_notification,
        .user_id = user_id
    };
    ESP_LOGI(TAG, "Default humidity: %d", (int)(limits.humidity_limit * 10));
    ESP_LOGI(TAG, "Default temperature: %d", (int)(limits.temperature_limit * 10));
    ESP_LOGI(TAG, "User ID: %s", limits.user_id);
    return limits;
}