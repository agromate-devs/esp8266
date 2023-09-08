#include "mqtt_client.h"

extern TaskHandle_t temperature_task_handle;

typedef struct SensorTask {
    esp_mqtt_client_handle_t client;
    char *uuid;
} SensorTask;

typedef struct TemperatureTask {
    int temperature_limit;
    int humidity_limit;
} TemperatureTask;

void temperature_task(void *arg);
void init_sensors_mqtt(char *uuid, esp_mqtt_client_handle_t client);