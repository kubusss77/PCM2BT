#include "settings.h"
#include "esp_adc/adc_continuous.h"
#include "esp_gap_bt_api.h"


adc_continuous_handle_t adc_handle = NULL;

char sink_dev_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = "BTS800"; // Replace with your device name

bool connected = false;