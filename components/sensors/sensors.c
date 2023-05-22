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

#define DELAY(x) vTaskDelay(x / portTICK_PERIOD_MS)
#define GPIO_PIN 5
#define DHT_GPIO 5 // D1 pin

int *temp_history_hour;
int *humidity_history_hour;
uint16_t *soil_humidity_history_hour;
static int media_temp;
static int media_hum;
static char uuid_sensor[UUID_LEN];
static esp_mqtt_client_handle_t client_sensor;

void temperature_task(void *arg)
{
    int *temp_history_hour = malloc(sizeof(int) * 60);
    int *humidity_history_hour = malloc(sizeof(int) * 60);
    if (temp_history_hour == NULL || humidity_history_hour == NULL)
    {
        ESP_LOGE("dht11", "Error allocating memory. Free memory %d", esp_get_free_heap_size());
    }

    ESP_ERROR_CHECK(dht_init(DHT_GPIO, false));
    DELAY(2000);
    int16_t humidity = 0;
    int16_t temperature = 0;
    int current_minute = 0;
    while (1)
    {
        if (dht_read_data(DHT_TYPE_DHT11, DHT_GPIO, &humidity, &temperature) == ESP_OK)
        {
            if (subscribed)
            {
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
        DELAY(60000); // Delay for 1 minute
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
            DELAY(60000);
            current_minute++;
        }
    }
}

void init_sensors_mqtt(char *uuid, esp_mqtt_client_handle_t client)
{
    strcpy(uuid_sensor, uuid);
    client_sensor = client;
    temp_history_hour = malloc(sizeof(int) * 60);
    humidity_history_hour = malloc(sizeof(int) * 60);
    soil_humidity_history_hour = malloc(sizeof(uint16_t) * 60);
    xTaskCreate(temperature_task, "temperature task", 2048, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(read_hygrometer, "hygrometer task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}