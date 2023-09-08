#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// FreeRTOS/esp-idf library
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "mqtt_client.h"

// Our components
#include "smartconfig.h"
#include "keystore.h"
#include "wifi.h"
#include "dht.h"
#include "delay.h"
#include "json.h"
#include "uuid.h"
#include "mqtt_helper.h"
// #include "sensors.h"
#include "reset.h"
#include "plant_manager.h"

#define TAG "main"
#define DEBUG 0

static char *uuid = "";

void initialize_reset_btn()
{
    gpio_config_t io_conf;

    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    // create a queue to handle gpio event from isr
    interputQueue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(LED_Control_Task, "gpio_task_example", 2048, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(0);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, wifi_reset_interrupt_handler, (void *)GPIO_INPUT_IO_0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    initialize_reset_btn(); // Initialize button btn first to avoid block during wifi sta

    if (is_already_registered())
    {
        ESP_LOGI(TAG, "Already registered, trying to connect");
        init_wifi_connection();
    }
    else
    {
        ESP_LOGW(TAG, "Starting smartconfig");
        initialise_smartconfig(0);
    }

    if (uuid_exists())
    {
        uuid = malloc(sizeof(char) * UUID_LEN);
        uuid = read_key("uuid", UUID_LEN);
    }
    
    mqtt_app_start();
}