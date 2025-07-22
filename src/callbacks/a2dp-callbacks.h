#pragma once
#include "esp_log.h"
#include "esp_a2dp_api.h"

void a2dp_event_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

int32_t a2dp_data_callback(uint8_t *data, int32_t len);
