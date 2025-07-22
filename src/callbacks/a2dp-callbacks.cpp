#include <math.h>

// Parametry sygnału testowego
#define TEST_SAMPLE_RATE 44100
#define TEST_FREQ 100.0f
#define TEST_AMPLITUDE 8000
static float test_phase = 0.0f;

#include "a2dp-callbacks.h"
#include "esp_log.h"
#include "esp_a2dp_api.h"

void a2dp_event_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT:
            ESP_LOGI("a2dp_callback", "Connection state %d", param->conn_stat.state);
            if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                ESP_LOGI("a2dp_callback", "Sending MEDIA_CTRL_START");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
            }
            break;
        case ESP_A2D_AUDIO_STATE_EVT:
            ESP_LOGI("a2dp_callback", "Audio state %d", param->audio_stat.state);
            break;
        case ESP_A2D_AUDIO_CFG_EVT:
            ESP_LOGI("a2dp_callback", "Audio config, codec type %d", param->audio_cfg.mcc.type);
            break;
        case ESP_A2D_PROF_STATE_EVT:
            ESP_LOGI("a2dp_callback", "Profile state event");
            break;
        case ESP_A2D_MEDIA_CTRL_ACK_EVT:
            ESP_LOGI("a2dp_callback", "Media ctrl ack %d", param->media_ctrl_stat.cmd);
            break;
        default:
            ESP_LOGI("a2dp_callback", "Event %d", event);
            break;
    }
}

int32_t a2dp_data_callback(uint8_t *data, int32_t len) {
    ESP_LOGD("a2dp_data_callback", "Data callback called");

    // Generowanie 100 Hz sinusa, 16-bit mono, little-endian
    int16_t *out = (int16_t *)data;
    int samples = len / sizeof(int16_t);
    float phase_inc = 2.0f * 3.14159265f * TEST_FREQ / TEST_SAMPLE_RATE;
    for (int i = 0; i < samples; ++i) {
        out[i] = (int16_t)(sinf(test_phase) * TEST_AMPLITUDE);
        test_phase += phase_inc;
        if (test_phase > 2.0f * 3.14159265f) test_phase -= 2.0f * 3.14159265f;
    }
    return samples * sizeof(int16_t);
}