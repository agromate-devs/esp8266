#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "esp_log.h"
#include "keystore.h"

#define TAG "WIFI Reset"

xQueueHandle interputQueue;

void IRAM_ATTR wifi_reset_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interputQueue, &pinNumber, NULL);
}

void LED_Control_Task(void *params)
{
    int pinNumber;
    while (true)
    {
        if (xQueueReceive(interputQueue, &pinNumber, portMAX_DELAY))
        {
            ESP_LOGW(TAG, "Resetting wifi");
            // TODO: Toggle LED status
	        clear_key("uuid");
		clear_key("current_plant");	// Remove the plant registered to sensor
            reset_wifi_credentials();
        }
    }
}
