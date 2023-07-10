/*
*   Code for aqua pump and ultrasonic vaporizator
*   
*/

#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DELAY(x) vTaskDelay(x / portTICK_PERIOD_MS)

#define VAPORIZER_PIN 14
#define WATER_PUMP_PIN 15

#define DEFAULT_VAPORIZER_TIME 20000    // TODO: We will tweak in future
#define DEFAULT_WATER_PUMP_TIME 20000

// // float default_temperature = 0.0;
// float temperature_limit = 0.0;
// int notify_wrong_temperature = 0;    // Boolean flag

float default_humidity = 0.0;
float humidity_limit = 0.0;
int notify_wrong_humidity = 0;    // Boolean flag

float default_precipitation = 0.0;
float precipitation_limit = 0.0;
int notify_wrong_precipitation = 0;    // Boolean flag

// int upload_done = 0; // Boolean flag


static int wrong_temperature_retry = 0; // Number of times that the temperature is wrong
static int wrong_humidity_retry = 0; // Number of times that the humidity is wrong
static int wrong_precipitation_retry = 0; // Number of times that the precipitation is wrong

void pump(gpio_num_t gpio_number, int irrigate_for) // Run as a task
{
    gpio_set_level(gpio_number, 1);
    DELAY(irrigate_for);
    gpio_set_level(gpio_number, 0);
}

void listener()
{
    while (1)
    {
        if( (humidity_limit - 5) > default_humidity || default_humidity > (humidity_limit + 5)) { // Range +5 -5
            wrong_humidity_retry++;
            pump(VAPORIZER_PIN, DEFAULT_VAPORIZER_TIME); //   Run vaporizer for 10 seconds by default
            if(wrong_humidity_retry == 3 && notify_wrong_humidity){  // Send notification to user

            }
        }

        if((precipitation_limit - 5) > default_precipitation || default_precipitation > (precipitation_limit + 5)) {
            wrong_precipitation_retry++;
            pump(WATER_PUMP_PIN, DEFAULT_WATER_PUMP_TIME); // Run pump for 20 seconds by default
            if(wrong_precipitation_retry == 3 && notify_wrong_precipitation){  // Send notification to user

            }
        }
    }
    
}