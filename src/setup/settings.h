#pragma once
#include "driver/i2s_std.h"
#include "esp_gap_bt_api.h"
#include "freertos/FreeRTOS.h"



extern i2s_port_t i2s_port;
extern i2s_chan_handle_t i2s_rx_handle;
extern uint32_t i2s_sample_rate;
extern gpio_num_t i2s_bck_io;
extern gpio_num_t i2s_ws_io;
extern gpio_num_t i2s_di_io;

extern char sink_dev_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
extern bool connected;

extern uint8_t i2s_audio_buffer[1024];
extern volatile size_t i2s_audio_buffer_size;
extern volatile bool i2s_audio_buffer_ready;
extern SemaphoreHandle_t i2s_buffer_mutex;