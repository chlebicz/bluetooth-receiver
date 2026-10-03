#pragma once

#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "core_handle.h"
#include "esp_log.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "bt_app_core_utils.h"
#include <string.h>

// Bluetooth callbacks
void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
void bt_app_avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
void bt_app_dev_cb(esp_bt_dev_cb_event_t event, esp_bt_dev_cb_param_t *param);

// Audio processing callback
void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);

// Helpers
void a2dp_watchdog_cb(TimerHandle_t xTimer);
bool is_device_bonded(esp_bd_addr_t bda);
void handle_audio_state_change(esp_a2d_audio_state_t a2d_state);
void handle_track_info_update(trackinfoupdate_type_t type, uint8_t *value);
void send_media_command(uint8_t command);
void print_esp_mac_address(void);
