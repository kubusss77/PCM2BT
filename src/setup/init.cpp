#include "init.h"
#include "settings.h"
#include "esp_log.h"
#include "esp_err.h"

#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_tdm.h"

#include "esp_bt.h" // BT controller
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_bt_main.h" // Bluedroid

#include "callbacks/a2dp-callbacks.h"
#include "callbacks/gap-callbacks.h"



esp_err_t init_i2s() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_port, I2S_ROLE_MASTER);
    i2s_rx_handle = NULL;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL,&i2s_rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(i2s_sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = GPIO_NUM_0,
            .bclk = i2s_bck_io,
            .ws = i2s_ws_io,
            .dout = I2S_GPIO_UNUSED,
            .din = i2s_di_io,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_rx_handle));

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

