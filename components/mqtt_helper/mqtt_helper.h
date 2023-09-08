#include "mqtt_client.h"

#define SUB "esp8266/sub"
#define PUB "esp8266/pub"

extern int connected;
void mqtt_app_start(void);
extern esp_mqtt_client_handle_t client;
void mqtt_task(void *arg);