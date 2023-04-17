#include "cJSON.h"
#include <stdio.h>

char *create_dht_json(int temp, int hum){
    char *json_string = NULL;
    cJSON *message = cJSON_CreateObject();
    if (message == NULL)
    {
        goto end;
    }
    
    cJSON *temperature = cJSON_CreateNumber((temp / 10));
    cJSON *humidity = cJSON_CreateNumber((hum / 10));

    if(temperature == NULL || humidity == NULL)
        goto end;

    cJSON_AddItemToObject(message, "temperature", temperature);
    cJSON_AddItemToObject(message, "humidity", humidity);
    json_string = cJSON_Print(message);
    if (json_string == NULL)
    {
        fprintf(stderr, "Failed to print monitor.\n");
    }

end:
    cJSON_Delete(message);
    return json_string;
}