#include "mqtt_client.h"

void init_sensors_mqtt();

typedef struct SensorTask {
    esp_mqtt_client_handle_t client;
    char *uuid;
} SensorTask;