#pragma once
#include "esp_adc/adc_continuous.h"
#include "esp_gap_bt_api.h"


extern adc_continuous_handle_t adc_handle;

extern char sink_dev_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];

extern bool connected;