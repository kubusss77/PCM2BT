#include <math.h>
// Funkcja generująca 16-bitowy sinus stereo (A=440Hz, Fs=44100Hz)
void generate_test_tone(uint8_t* out, size_t samples) {
    static float phase = 0.0f;
    float freq = 440.0f;
    float sample_rate = 44100.0f;
    float phase_inc = 2.0f * 3.14159265f * freq / sample_rate;
    for (size_t i = 0; i < samples; ++i) {
        int16_t val = (int16_t)(sinf(phase) * 12000.0f);
        // stereo: L
        out[4*i+0] = val & 0xFF;
        out[4*i+1] = (val >> 8) & 0xFF;
        // stereo: R
        out[4*i+2] = val & 0xFF;
        out[4*i+3] = (val >> 8) & 0xFF;
        phase += phase_inc;
        if (phase > 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
    }
}
#include "i2s_task.h"
#include "setup/settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2s_std.h"



void conv24to16bits(const uint8_t* in, size_t in_bytes, uint8_t* out, size_t* out_bytes) {
    size_t samples = in_bytes / 8;
    const uint8_t* src = in;
    uint8_t* dst = out;

    for (size_t i = 0; i < samples; ++i) {
        //Left channel
        int32_t left = (src[2] << 24) | (src[1] << 16) | (src[0] << 8);
        int16_t left16 = (int16_t)(left >> 16);
        dst[0] = left16 & 0xFF;
        dst[1] = (left16 >> 8) & 0xFF;

        //Right channel
        int32_t right = (src[6] << 24) | (src[5] << 16) | (src[4] << 8);
        int16_t right16 = (int16_t)(right >> 16);
        dst[2] = right16 & 0xFF;
        dst[3] = (right16 >> 8) & 0xFF;

        src += 8;
        dst += 4;
    }

    *out_bytes = samples * 4;
}

void i2s_task(void* pvParameters) {
    ESP_LOGI("i2s_task", "Starting I2S task");

    // TEST: generuj czysty ton zamiast danych z I2S
    // Jeśli chcesz wrócić do I2S, zakomentuj poniższy blok i odkomentuj oryginalny
    while (true) {
        if (!connected) {
            ESP_LOGI("i2s_task", "BT disconnected, ending I2S task");
            break;
        }
        // 256 próbek stereo = 1024 bajty (2x16bit x 256)
        xSemaphoreTake(i2s_buffer_mutex, portMAX_DELAY);
        generate_test_tone(i2s_audio_buffer, 256);
        i2s_audio_buffer_size = 1024;
        i2s_audio_buffer_ready = true;
        xSemaphoreGive(i2s_buffer_mutex);
        ESP_LOGI("i2s_task", "Generated test tone, first 8: %02x %02x %02x %02x %02x %02x %02x %02x",
            i2s_audio_buffer[0], i2s_audio_buffer[1], i2s_audio_buffer[2], i2s_audio_buffer[3],
            i2s_audio_buffer[4], i2s_audio_buffer[5], i2s_audio_buffer[6], i2s_audio_buffer[7]);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}