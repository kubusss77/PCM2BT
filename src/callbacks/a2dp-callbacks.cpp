#include "a2dp-callbacks.h"
#include "setup/settings.h"
#include "esp_a2dp_api.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"



void a2dp_event_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT:
            ESP_LOGI("a2dp_callback", "Connection state %d", param->conn_stat.state);
            if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
                ESP_LOGI("a2dp_callback", "Sending MEDIA_CTRL_START");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
            }
            if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
                ESP_LOGI("a2dp_callback", "Disconnected from %02x:%02x:%02x:%02x:%02x:%02x",
                         param->conn_stat.remote_bda[0], param->conn_stat.remote_bda[1],
                         param->conn_stat.remote_bda[2], param->conn_stat.remote_bda[3],
                         param->conn_stat.remote_bda[4], param->conn_stat.remote_bda[5]);
                connected = false;
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

    if (!connected) {
        memset(data, 0, len); // cisza
        return len;
    }

    if (i2s_audio_buffer_ready && i2s_audio_buffer_size > 0) {
        size_t to_copy = (len < i2s_audio_buffer_size) ? len : i2s_audio_buffer_size;
        memcpy(data, i2s_audio_buffer, to_copy);
        i2s_audio_buffer_ready = false;
        return to_copy;
    } else {
        // SZYBKA CISZA: kopiuj z gotowego statycznego zera
        int copied = 0;
        xSemaphoreTake(i2s_buffer_mutex, portMAX_DELAY);
        static const uint8_t zero[512] = {0};
        copied = 0;
        while (copied < (size_t)len) {
            size_t chunk = ((len - copied) > sizeof(zero)) ? sizeof(zero) : (len - copied);
            memcpy(data + copied, zero, chunk);
            copied += chunk;
        }
        xSemaphoreGive(i2s_buffer_mutex);
        return len;
    }
}