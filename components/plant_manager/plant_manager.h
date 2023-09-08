#include "freertos/task.h"
#include "sensor.h"

extern TaskHandle_t plant_task_handle;
extern int plant_assigned;

void plant_task(void *arg);
TemperatureTask parse_raw_response(char *response);