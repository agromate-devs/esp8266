#include "esp_log.h"
#include "cJSON.h"
#include <stdio.h>

char *create_dht_json(int temp, int hum, int soil_hum, char *uuid, int is_humidity_notification, int is_temp_notification, char *user_id){
    char *json_string = NULL;
    cJSON *message = cJSON_CreateObject();
    if (message == NULL)
    {
        goto end;
    }
    
    cJSON *temperature = cJSON_CreateNumber((temp / 10));
    cJSON *humidity = cJSON_CreateNumber((hum / 10));
    cJSON *soil_humidity = cJSON_CreateNumber((soil_hum / 10));
    cJSON *hour = cJSON_CreateBool(1);
    cJSON *media_month = cJSON_CreateBool(0);
    cJSON *uuid_json = cJSON_CreateString(uuid);
    cJSON *user_id_json = cJSON_CreateString(user_id);
    cJSON *is_temp_notification_json = cJSON_CreateBool(is_temp_notification);
    cJSON *is_humidity_notification_json = cJSON_CreateBool(is_humidity_notification);

    ESP_LOGI("json", "User ID: %s", user_id);
    if(temperature == NULL || humidity == NULL)
        goto end;

    /*
        If this message is a notification, 
        add user_id and others info so IoT core can detect msg
        and lambda can send it to the correct user
    */
    if(is_temp_notification || is_humidity_notification) {
        cJSON_AddItemToObject(message, "user_id", user_id_json);
        cJSON_AddItemToObject(message, "is_temperature_notification", is_temp_notification_json);
        cJSON_AddItemToObject(message, "is_humidity_notification", is_humidity_notification_json);
    }
    cJSON_AddItemToObject(message, "temperature", temperature);
    cJSON_AddItemToObject(message, "humidity", humidity);
    cJSON_AddItemToObject(message, "soil_humidity", soil_humidity);
    cJSON_AddItemToObject(message, "hour", hour);
    cJSON_AddItemToObject(message, "media_month", media_month);
    cJSON_AddItemToObject(message, "uuid", uuid_json);
    
    json_string = cJSON_Print(message);
    if (json_string == NULL)
    {
        fprintf(stderr, "Failed to print monitor.\n");
    }

end:
    cJSON_Delete(message);
    return json_string;
}