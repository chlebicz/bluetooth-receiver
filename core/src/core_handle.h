#pragma once
#include "core/core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_bt_defs.h"

#define CORE_MAX_TRACKINFOUPDATE_CBS 2
#define CORE_MAX_AUDIO_STATE_CBS     2
#define CORE_MAX_CONNECTION_STATE_CBS 2

typedef struct core_handle *core_handle_t;

struct core_handle
{
  track_info_update_cb_t track_info_update_cbs[CORE_MAX_TRACKINFOUPDATE_CBS];
  void *track_info_update_args[CORE_MAX_TRACKINFOUPDATE_CBS];

  audio_state_update_cb_t audio_state_update_cbs[CORE_MAX_AUDIO_STATE_CBS];
  void *audio_state_update_args[CORE_MAX_AUDIO_STATE_CBS];

  connection_state_update_cb_t connection_state_update_cbs[CORE_MAX_CONNECTION_STATE_CBS];
  void *connection_state_update_args[CORE_MAX_CONNECTION_STATE_CBS];

  volatile int is_a2dp_connected;
  volatile int is_a2dp_connecting;
  esp_bd_addr_t peer_bda;
  volatile uint8_t avrc_tl;

  TimerHandle_t a2dp_watchdog_timer;
  esp_bd_addr_t current_acl_bda;
};

core_handle_t get_core_handle(void);
