#include "gap-callbacks.h"
#include "esp_log.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include <string.h>
#include "setup/settings.h"

#include "freertos/FreeRTOS.h"
#include "tasks/i2s_task.h"


const char* gap_event_name(esp_bt_gap_cb_event_t event) {
    switch(event) {
        case ESP_BT_GAP_DISC_RES_EVT: return "DISC_RES_EVT";
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: return "DISC_STATE_CHANGED_EVT";
        case ESP_BT_GAP_RMT_SRVCS_EVT: return "RMT_SRVCS_EVT";
        case ESP_BT_GAP_RMT_SRVC_REC_EVT: return "RMT_SRVC_REC_EVT";
        case ESP_BT_GAP_AUTH_CMPL_EVT: return "AUTH_CMPL_EVT";
        case ESP_BT_GAP_PIN_REQ_EVT: return "PIN_REQ_EVT";
        case ESP_BT_GAP_CFM_REQ_EVT: return "CFM_REQ_EVT";
        case ESP_BT_GAP_KEY_NOTIF_EVT: return "KEY_NOTIF_EVT";
        case ESP_BT_GAP_KEY_REQ_EVT: return "KEY_REQ_EVT";
        case ESP_BT_GAP_READ_RSSI_DELTA_EVT: return "READ_RSSI_DELTA_EVT";
        case ESP_BT_GAP_MODE_CHG_EVT: return "MODE_CHG_EVT";
        default: return "UNKNOWN_EVT";
    }
}

void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    ESP_LOGD("gap_callback", "GAP event: %d (%s)", event, gap_event_name(event));

    switch (event) {
        case ESP_BT_GAP_DISC_RES_EVT: {
            if (param->disc_res.num_prop == 0) {
                ESP_LOGI("gap_callback", "No properties found in discovery result");
                return;
            }

            char dev_mac_addr[18];
            uint8_t *bda = param->disc_res.bda;
            snprintf(dev_mac_addr, sizeof(dev_mac_addr), "%02x:%02x:%02x:%02x:%02x:%02x",
                     bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

            char dev_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = {0};
            bool name_found = false;

            for (int i = 0; i < param->disc_res.num_prop; i++) {
                if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_BDNAME) {
                    int len = param->disc_res.prop[i].len;
                    if (len > ESP_BT_GAP_MAX_BDNAME_LEN) len = ESP_BT_GAP_MAX_BDNAME_LEN;
                    ESP_LOGI("gap_callback", "BDNAME len: %d", len);
                    ESP_LOG_BUFFER_HEX_LEVEL("gap_callback", (const void*)param->disc_res.prop[i].val, len, ESP_LOG_DEBUG);
                    memcpy(dev_name, param->disc_res.prop[i].val, len);
                    dev_name[len] = 0;
                    name_found = true;
                    break;
                }
            }

            if (!name_found) {
                for (int i = 0; i < param->disc_res.num_prop; i++) {
                    if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR) {
                        ESP_LOGD("gap_callback", "EIR data present");
                        ESP_LOG_BUFFER_HEX_LEVEL("gap_callback", (const void*)param->disc_res.prop[i].val, param->disc_res.prop[i].len, ESP_LOG_DEBUG);
                        uint8_t name_len = 0;
                        uint8_t *eir_name = esp_bt_gap_resolve_eir_data((uint8_t *)param->disc_res.prop[i].val, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &name_len);
                        if (eir_name && name_len > 0 && name_len <= ESP_BT_GAP_MAX_BDNAME_LEN) {
                            memcpy(dev_name, eir_name, name_len);
                            dev_name[name_len] = 0;
                            name_found = true;
                        }
                        break;
                    }
                }
            }
            if (name_found && dev_name[0] != '\0') {
                ESP_LOGI("gap_callback", "Znaleziono: %s, nazwa: %s", dev_mac_addr, dev_name);
            } else {
                ESP_LOGI("gap_callback", "Znaleziono: %s, brak nazwy urządzenia", dev_mac_addr);
            }

            if (strcmp(dev_name, sink_dev_name) == 0) {
                if (esp_a2d_source_connect(bda) == ESP_OK) {
                    ESP_LOGI("gap_callback", "Połączono z: %s (%s)", dev_name, dev_mac_addr);
                    ESP_ERROR_CHECK(esp_bt_gap_cancel_discovery());
                    connected = true;

                    // Start I2S task after successful connection
                    xTaskCreate(i2s_task, "i2s_task", 4096, NULL, 5, NULL);
                } else {
                    ESP_LOGE("gap_callback", "Failed to connect to %s (%s)", dev_name, dev_mac_addr);
                }
            }
            break;
        }
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                ESP_LOGI("gap_callback", "Discovery finished");
            } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
                ESP_LOGI("gap_callback", "Discovery started");
            }
            break;
        }
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            ESP_LOGI("gap_callback", "Authentication complete");
            break;
        case ESP_BT_GAP_PIN_REQ_EVT:
            ESP_LOGI("gap_callback", "PIN request");
            break;
        case ESP_BT_GAP_CFM_REQ_EVT:
            ESP_LOGI("gap_callback", "User confirmation request");
            break;
        case ESP_BT_GAP_KEY_NOTIF_EVT:
            ESP_LOGI("gap_callback", "Passkey notification");
            break;
        case ESP_BT_GAP_KEY_REQ_EVT:
            ESP_LOGI("gap_callback", "Passkey request");
            break;
        case ESP_BT_GAP_READ_RSSI_DELTA_EVT:
            ESP_LOGI("gap_callback", "Read RSSI delta");
            break;
        case ESP_BT_GAP_MODE_CHG_EVT:
            ESP_LOGI("gap_callback", "Mode change event");
            break;
        case ESP_BT_GAP_RMT_SRVCS_EVT:
            ESP_LOGI("gap_callback", "Remote services event");
            break;
        case ESP_BT_GAP_RMT_SRVC_REC_EVT:
            ESP_LOGI("gap_callback", "Remote service record event");
            break;
        case ESP_BT_GAP_REMOVE_BOND_DEV_COMPLETE_EVT:   
            ESP_LOGI("gap_callback", "Bond device removed");
            break;
        case ESP_BT_GAP_QOS_CMPL_EVT:
            ESP_LOGI("gap_callback", "QoS complete event");
            break;
        case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
            ESP_LOGI("gap_callback", "ACL connection complete status event");
            break;
        case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
            connected = false;
            ESP_LOGI("gap_callback", "ACL disconnection complete status event");
            break;
        default:
            ESP_LOGI("gap_callback", "Unhandled GAP event: %d", event);
            break;
    }   
}