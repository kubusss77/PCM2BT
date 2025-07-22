// PCM2BT

#include "setup/init.h"
#include "setup/settings.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_continuous.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_a2dp_api.h"
#include "esp_gap_bt_api.h"
#include "callbacks/a2dp-callbacks.h"
#include <string.h>


extern "C" void app_main() {
    //esp_log_level_set("*", ESP_LOG_DEBUG);
    esp_log_level_set("a2dp_data_callback", ESP_LOG_DEBUG);

    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI("main", "Start PCM2BT");

    if (init_adc() != ESP_OK) {
        ESP_LOGE("main", "ADC initialization failed");
        return;
    }

    if (init_bt() != ESP_OK) {
        ESP_LOGE("main", "Classic Bluetooth initialization failed");
        return;
    }

    if (reg_cb() != ESP_OK) {
        ESP_LOGE("main", "Failed to register Bluetooth callbacks");
        return;
    }

    ESP_LOGI("main", "Bluetooth scanning started");
    if (esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE) != ESP_OK) {
        ESP_LOGE("main", "Failed to set scan mode");
        return;
    }

    esp_err_t scan_ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 30, 0);
    if (scan_ret != ESP_OK) {
        ESP_LOGE("main", "Failed to start discovery: %s", esp_err_to_name(scan_ret));
        return;
    }


    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}