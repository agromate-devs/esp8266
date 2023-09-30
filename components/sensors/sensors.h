#include "mqtt_client.h"

extern TaskHandle_t temperature_task_handle;

typedef struct SensorTask {
    esp_mqtt_client_handle_t client;
    char *uuid;
} SensorTask;

typedef struct Plant {
    int temperature_limit;
    int humidity_limit;
    char *user_id;
    int temperature_notification;
    int humidity_notification;
} Plant;

void temperature_task(void *arg);
void init_sensors_mqtt(char *uuid, esp_mqtt_client_handle_t client);
extern int plant_assigned;