#include "settings.h"
#include "driver/i2s_std.h"
#include "esp_gap_bt_api.h"
#include "freertos/semphr.h"



i2s_port_t i2s_port = I2S_NUM_0;
i2s_chan_handle_t i2s_rx_handle = NULL;
uint32_t i2s_sample_rate = 44100;
gpio_num_t i2s_bck_io = GPIO_NUM_26;
gpio_num_t i2s_ws_io = GPIO_NUM_25;
gpio_num_t i2s_di_io = GPIO_NUM_22;

char sink_dev_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = "Colour Sound";

bool connected = false;

SemaphoreHandle_t i2s_buffer_mutex = NULL;

uint8_t i2s_audio_buffer[1024];
volatile size_t i2s_audio_buffer_size = 0;
volatile bool i2s_audio_buffer_ready = false;