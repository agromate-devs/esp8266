#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#define STORAGE "storage"
#define TAG "Keystore"

char *read_key(char *key, int length)
{
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(STORAGE, NVS_READWRITE, &my_handle));
    // Read
    char *value = malloc(length * sizeof(char));
    size_t buf_len = length;
    uint16_t err = nvs_get_str(my_handle, key, value, &buf_len);
    switch (err)
    {
    case ESP_OK:
#ifdef DEBUG
        ESP_LOGI(TAG, "Value %s", value);
#endif
        return value;
        break;
    case ESP_ERR_NVS_NOT_FOUND:
#ifdef DEBUG
        ESP_LOGI(TAG, "The value is not initialized yet!\n");
#endif
        break;
    default:
        ESP_LOGI(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    // Close
    nvs_close(my_handle);
    return NULL;
}

void write_key(char *key, char *value){
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(STORAGE, NVS_READWRITE, &my_handle));
#ifdef DEBUG
    ESP_LOGI(TAG, "Write %s = %s to NVS", key, value);
#endif
    ESP_ERROR_CHECK(nvs_set_str(my_handle, key, value));

    ESP_ERROR_CHECK(nvs_commit(my_handle));
}

void clear_key(char *key){
    nvs_handle_t my_handle;
    ESP_ERROR_CHECK(nvs_open(STORAGE, NVS_READWRITE, &my_handle));
    nvs_erase_key(my_handle, key);
}