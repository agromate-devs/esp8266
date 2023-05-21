#include "cJSON.h"
#include <stdio.h>

char *create_dht_json(int temp, int hum, char *uuid){
    char *json_string = NULL;
    cJSON *message = cJSON_CreateObject();
    if (message == NULL)
    {
        goto end;
    }
    
    cJSON *temperature = cJSON_CreateNumber((temp / 10));
    cJSON *humidity = cJSON_CreateNumber((hum / 10));
    cJSON *hour = cJSON_CreateBool(1);
    cJSON *media_month = cJSON_CreateBool(0);
    cJSON *uuid_json = cJSON_CreateString(uuid);

    if(temperature == NULL || humidity == NULL)
        goto end;

    cJSON_AddItemToObject(message, "temperature", temperature);
    cJSON_AddItemToObject(message, "humidity", humidity);
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