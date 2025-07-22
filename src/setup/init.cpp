#include "init.h"
#include "settings.h"
#include "esp_log.h"
#include "esp_err.h"

#include "soc/soc_caps.h" // Min/Max sample frequency
#include "esp_adc/adc_continuous.h"

#include "esp_bt.h" // BT controller
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt_main.h" // Bluedroid

#include "callbacks/a2dp-callbacks.h"
#include "callbacks/gap-callbacks.h"



esp_err_t init_adc() {
    ESP_LOGI("init_adc","Initializing ADC in continuous mode...");

    ESP_LOGD("init_adc"," Min sample freq: %d", SOC_ADC_SAMPLE_FREQ_THRES_LOW);
    ESP_LOGD("init_adc"," Max sample freq: %d", SOC_ADC_SAMPLE_FREQ_THRES_HIGH);

    adc_continuous_handle_cfg_t adc_handle_config = {
        .max_store_buf_size = 1024 * sizeof(int16_t),
        .conv_frame_size = 1024 * sizeof(int16_t),
        .flags = 0,
    };
    ESP_LOGD("init_adc","Creating ADC handle");
    if (adc_continuous_new_handle(&adc_handle_config, &adc_handle) != ESP_OK) {
        ESP_LOGE("init_adc", "Failed to create ADC handle");
        return ESP_FAIL;
    }

    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN_DB_12,
        .channel = ADC_CHANNEL_0,
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };
    adc_continuous_config_t adc_config = {
        .pattern_num = 1,
        .adc_pattern = &adc_pattern,
        .sample_freq_hz = 32000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };
    ESP_LOGD("init_adc","Configuring ADC in continuous mode");
    if (adc_continuous_config(adc_handle, &adc_config) != ESP_OK) {
        ESP_LOGE("init_adc", "Failed to configure ADC");
        return ESP_FAIL;
    }

    ESP_LOGD("init_adc","Starting ADC in continuous mode");
    if (adc_continuous_start(adc_handle) != ESP_OK) {
        ESP_LOGE("init_adc", "Failed to start ADC");
        return ESP_FAIL;
    }
    

    ESP_LOGI("init_adc","ADC configured and started successfully");
    return ESP_OK;
}

esp_err_t init_bt() {
    esp_err_t ret;

    ESP_LOGI("init_bt", "Initializing Classic Bluetooth...");

    esp_bt_controller_status_t bt_status = esp_bt_controller_get_status();
    if (bt_status != ESP_BT_CONTROLLER_STATUS_IDLE) {
        ESP_LOGE("init_bt", "BT controller already initialized or in wrong state (%d)", bt_status);
        return ESP_FAIL;
    }

    ESP_LOGD("init_bt", "Initializing NVS flash");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW("init_bt", "NVS flash needs to be erased, re-initializing");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    ESP_LOGD("init_bt", "Releasing BLE memory");
    ESP_ERROR_CHECK(esp_bt_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bluetooth_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (bluetooth_cfg.mode != ESP_BT_MODE_CLASSIC_BT) {
        ESP_LOGE("init_bt", "Invalid Bluetooth controller mode: %d", bluetooth_cfg.mode);
        return ESP_FAIL;
    }

    ESP_LOGD("init_bt", "Initializing Bluetooth controller");
    ret = esp_bt_controller_init(&bluetooth_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE("init_bt", "Failed to initialize Bluetooth controller: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGD("init_bt", "Enabling Bluetooth controller");
    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK) {
        ESP_LOGE("init_bt", "Failed to enable Bluetooth controller: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }


    ESP_LOGD("init_bt", "Initializing Bluedroid");
    ESP_ERROR_CHECK(esp_bluedroid_init());

    ESP_LOGD("init_bt", "Enabling Bluedroid");
    ESP_ERROR_CHECK(esp_bluedroid_enable());


    ESP_LOGI("init_bt", "Classic Bluetooth initialization completed successfully");
    return ESP_OK;
}


esp_err_t reg_cb() {
    ESP_LOGI("reg_callbacks", "Registering GAP and A2DP callbacks...");

    if (esp_bt_gap_register_callback(gap_callback) != ESP_OK) {
        ESP_LOGE("reg_callbacks", "Failed to register GAP callback");
        return ESP_FAIL;
    }
    if (esp_a2d_register_callback(a2dp_event_callback) != ESP_OK) {
        ESP_LOGE("reg_callbacks", "Failed to register A2DP callback");
        return ESP_FAIL;
    }
    if (esp_a2d_source_register_data_callback(a2dp_data_callback) != ESP_OK) {
        ESP_LOGE("reg_callbacks", "Failed to register A2DP data callback");
        return ESP_FAIL;
    }

    if (esp_a2d_source_init() != ESP_OK) {
        ESP_LOGE("reg_callbacks", "Failed to initialize A2DP source");
        return ESP_FAIL;
    }

    ESP_LOGI("reg_callbacks", "Callbacks registered successfully");
    return ESP_OK;
}

