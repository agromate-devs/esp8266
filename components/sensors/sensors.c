#include <driver/gpio.h>
#include <driver/adc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_client.h"
#include "../mqtt_helper/mqtt_helper.h"
#include "../json/json.h"
#include "dht.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include "../uuid/uuid.h"
#include "sensors.h"
#include "rom/gpio.h"

#define DELAY(x) vTaskDelay(x / portTICK_PERIOD_MS)
#define GPIO_PIN 5
#define DHT_GPIO 5 // D1 pin
#define HUMIDIFIER_GPIO GPIO_NUM_14   // D5 pin

#define TOLLERANCE 10
#define VALUE_IN_TOLLERANCE(x, a, tollerance) (x > a + tollerance && x < a - tollerance)
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_NUM_14))

#define DEBUG_DHT_MQTT 1    // CHANGE IT IN PRODUCTION!
#define DELAY_TIME DEBUG_DHT_MQTT ? 10000 : 60000  // 10 seconds if we are in debug mode else 1 minute

int *temp_history_hour;
int *humidity_history_hour;
uint16_t *soil_humidity_history_hour;
static int media_temp;
static int media_hum;
static char uuid_sensor[UUID_LEN];
static esp_mqtt_client_handle_t client_sensor;

void init_humidifier() {
    gpio_pad_select_gpio(HUMIDIFIER_GPIO);
    gpio_set_direction (HUMIDIFIER_GPIO, GPIO_MODE_OUTPUT);
}

void start_humidifier() {
    gpio_set_level(HUMIDIFIER_GPIO, 1);
}

void stop_humidifier() {
    gpio_set_level(HUMIDIFIER_GPIO, 0);
}

void temperature_task(void *arg)
{
    int *temp_history_hour = malloc(sizeof(int) * 60);
    int *humidity_history_hour = malloc(sizeof(int) * 60);
    if (temp_history_hour == NULL || humidity_history_hour == NULL)
    {
        ESP_LOGE("dht11", "Error allocating memory. Free memory %d", esp_get_free_heap_size());
    }
    TemperatureTask *limits = (TemperatureTask *)arg;

    ESP_ERROR_CHECK(dht_init(DHT_GPIO, false));
    DELAY(2000);
    int16_t humidity = 0;
    int16_t temperature = 0;
    int current_minute = 0;
    init_humidifier();
    int humidifier = 0;
    while (1)
    {
        if (dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature) == ESP_OK)
        {
            ESP_LOGI("sensors", "Humidity: %d, Temperature: %d\n, HUM_LIMIT: %d, TEMP_LIMIT: %d", humidity, temperature, limits->humidity_limit, limits->temperature_limit);
            if(!VALUE_IN_TOLLERANCE(humidity, limits->humidity_limit, TOLLERANCE) && !humidifier) {
                // Start humidifier
                ESP_LOGI("sensors", "Start humidifier");
                start_humidifier();
                humidifier = 1;
                ESP_LOGI("sensors", "HUMIDIFIER_STATUS: %d\n", humidifier);
            } else if(humidifier && VALUE_IN_TOLLERANCE(humidity, limits->humidity_limit, TOLLERANCE)) {
                // Stop humidifier
                ESP_LOGI("sensors", "Stop humidifier");
                stop_humidifier();
                humidifier = 0;
            }
            if (subscribed)
            {
                if(DEBUG_DHT_MQTT) {
                    char *message = create_dht_json(temperature, humidity, 0, uuid_sensor);
                    esp_mqtt_client_publish(client_sensor, "sdk/test/python", message, 0, 0, 0);
                    ESP_LOGW("sensors", "DEV CODE DETECTED! If you are in production mode set DEBUG_DHT_MQTT to 0");
                }

                if (current_minute == 59)
                { // 60 finish to 59 since we start from 0
                    media_temp = 0;
                    media_hum = 0;
                    for (size_t i = 0; i < 60; i++)
                    {
                        media_temp += temp_history_hour[i];
                        media_hum += humidity_history_hour[i];
                    }
                    media_temp = media_temp / 60;
                    media_hum = media_hum / 60;
                    memset(temp_history_hour, 0, sizeof(int) * 60); // Cleanup array from old misurations
                    memset(humidity_history_hour, 0, sizeof(int) * 60);

                    // free(media_hum);
                    // free(media_temp);
                    current_minute = 0;
                }
                temp_history_hour[current_minute] = temperature;
                humidity_history_hour[current_minute] = humidity;
                current_minute++;
            }
        }
        else
        {
            ESP_LOGE("DHT11", "Fail to get dht temperature data\n");
        }
        DELAY(DELAY_TIME); // Delay for 1 minute
    }
    vTaskDelete(NULL);
}

void read_hygrometer(void *arg)
{
    int current_minute = 0;
    int media_soil_hum = 0;
    while (1)
    {
        if (subscribed)
        {
            if (current_minute == 59)
            {
                for (size_t i = 0; i < 60; i++)
                {
                    media_soil_hum += soil_humidity_history_hour[i];
                }
                char *message = create_dht_json(media_temp, media_hum, media_soil_hum, uuid_sensor);
                esp_mqtt_client_publish(client_sensor, PUB, message, 0, 0, 0);
                memset(soil_humidity_history_hour, 0, sizeof(int) * 60); // Cleanup array from old misurations
                media_soil_hum = 0;
                current_minute = 0;
            }
            gpio_set_level(GPIO_PIN, 1);
            DELAY(50);
            uint16_t *data = NULL;
            ESP_ERROR_CHECK(adc_read(data));
            gpio_set_level(GPIO_PIN, 0);
            soil_humidity_history_hour[current_minute] = *data;
            DELAY(DELAY_TIME);   // Delay for 1 minute
            current_minute++;
        }
    }
}

void init_sensors_mqtt(char *uuid, esp_mqtt_client_handle_t client, TemperatureTask *limits)
{
    strcpy(uuid_sensor, uuid);
    client_sensor = client;
    temp_history_hour = malloc(sizeof(int) * 60);
    humidity_history_hour = malloc(sizeof(int) * 60);
    soil_humidity_history_hour = malloc(sizeof(uint16_t) * 60);
    xTaskCreate(temperature_task, "temperature task", 2048, limits, tskIDLE_PRIORITY, NULL);
    // xTaskCreate(read_hygrometer, "hygrometer task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}