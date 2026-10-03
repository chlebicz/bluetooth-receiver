#include "bt_internal.h"
#include "a2dp_sink_common_utils.h"
#include "a2dp_utils_tags.h"
#include "a2dp_sink_int_codec_utils.h"
#include "bt_app_core_utils.h"
#include <string.h>

void handle_audio_state_change(
  esp_a2d_audio_state_t a2d_state
)
{
  core_handle_t hdl = get_core_handle();
  core_audio_state_t state = ESP_A2D_AUDIO_STATE_STARTED;
  if (a2d_state == ESP_A2D_AUDIO_STATE_STARTED)
  {
    state = CORE_AUDIO_STATE_PLAYING;
  }
  else if (a2d_state == ESP_A2D_AUDIO_STATE_SUSPEND)
  {
    state = CORE_AUDIO_STATE_PAUSED;
  }

  for (int i = 0; i < CORE_MAX_AUDIO_STATE_CBS; ++i)
  {
    if (hdl->audio_state_update_cbs[i] != NULL)
    {
      hdl->audio_state_update_cbs[i](
        state,
        hdl->audio_state_update_args[i]
      );
    }
  }
}

void handle_connection_state_change(
  esp_a2d_connection_state_t a2d_state
)
{
  core_handle_t hdl = get_core_handle();
  core_connection_state_t state = CORE_CONNECTION_STATE_DISCONNECTED;
  if (a2d_state == ESP_A2D_CONNECTION_STATE_CONNECTED)
  {
    state = CORE_CONNECTION_STATE_CONNECTED;
  }
  else if (a2d_state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
  {
    state = CORE_CONNECTION_STATE_DISCONNECTED;
  }
  else
  {
    return;
  }

  for (int i = 0; i < CORE_MAX_CONNECTION_STATE_CBS; ++i)
  {
    if (hdl->connection_state_update_cbs[i] != NULL)
    {
      hdl->connection_state_update_cbs[i](
        state,
        hdl->connection_state_update_args[i]
      );
    }
  }
}

void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
  core_handle_t hdl = get_core_handle();
  switch (event)
  {
  case ESP_A2D_PROF_STATE_EVT:
    if (param->a2d_prof_stat.init_state == ESP_A2D_INIT_SUCCESS) {
      ESP_LOGI("BT_AV", "A2DP Profile Ready! Opening for connections...");
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    }
    bt_app_work_dispatch(
      bt_a2d_evt_def_hdl, event, param, sizeof(esp_a2d_cb_param_t), NULL
    );
    break;
  case ESP_A2D_SNK_PSC_CFG_EVT:
  case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
  case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
  {
    bt_app_work_dispatch(
      bt_a2d_evt_def_hdl, event, param, sizeof(esp_a2d_cb_param_t), NULL
    );
    break;
  }

  case ESP_A2D_CONNECTION_STATE_EVT:
  {
    bt_app_work_dispatch(
      bt_a2d_evt_int_codec_hdl, event, param, sizeof(esp_a2d_cb_param_t), NULL
    );

    if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
    {
      if (hdl->a2dp_watchdog_timer) {
        xTimerStop(hdl->a2dp_watchdog_timer, 0);
      }
      ESP_LOGI("BT_AV", "Device Connected! Saving MAC to NVS...");
      hdl->is_a2dp_connected = 1;
      hdl->is_a2dp_connecting = 0;
      memcpy(hdl->peer_bda, param->conn_stat.remote_bda, sizeof(esp_bd_addr_t));

      // save to flash memory (NVS)
      nvs_handle_t my_handle;
      if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_blob(
          my_handle, "last_mac", hdl->peer_bda, sizeof(esp_bd_addr_t)
        );
        nvs_set_u8(my_handle, "wd_restarts", 0);
        nvs_commit(my_handle);
        nvs_close(my_handle);
      }

      esp_bt_gap_read_remote_name(param->conn_stat.remote_bda);
      handle_connection_state_change(param->conn_stat.state);
    }
    else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING)
    {
      hdl->is_a2dp_connecting = 1;
    }
    else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
    {
      ESP_LOGI("BT_AV", "Device Disconnected! Opening doors for reconnections.");
      hdl->is_a2dp_connected = 0;
      hdl->is_a2dp_connecting = 0;
      handle_connection_state_change(param->conn_stat.state);
    }
    else if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_ERROR)
    {
      // if (a2dp_watchdog_timer) {
      //     xTimerStop(a2dp_watchdog_timer, 0);
      // }
      // ESP_LOGI("BT_AV", "connection error. stopping watchdog timer");
    }
    break;
  }
  case ESP_A2D_AUDIO_STATE_EVT:
  {
    handle_audio_state_change(param->audio_stat.state);
    bt_app_work_dispatch(
      bt_a2d_evt_int_codec_hdl, event, param, sizeof(esp_a2d_cb_param_t), NULL
    );
    break;
  }
  case ESP_A2D_AUDIO_CFG_EVT:
  case ESP_A2D_SEP_REG_STATE_EVT:
  {
    bt_app_work_dispatch(
      bt_a2d_evt_int_codec_hdl, event, param, sizeof(esp_a2d_cb_param_t), NULL
    );
    break;
  }
  default:
    ESP_LOGE(BT_AV_TAG, "Invalid A2DP event: %d", event);
    break;
  }
}
