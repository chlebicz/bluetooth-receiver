#include "bt_internal.h"
#include <string.h>
#include "esp_log.h"

void send_media_command(uint8_t command) {
  core_handle_t hdl = get_core_handle();
  if (!hdl->is_a2dp_connected)
  {
    ESP_LOGW("BT_CTRL", "Cannot send command, no device connected.");
    return;
  }

  ESP_LOGI("BT_CTRL", "Sending AVRCP passthrough command: 0x%02x", command);
  // send button pressed
  hdl->avrc_tl++;
  hdl->avrc_tl &= 0x0F;
  esp_avrc_ct_send_passthrough_cmd(
    hdl->avrc_tl, command, ESP_AVRC_PT_CMD_STATE_PRESSED
  );

  vTaskDelay(pdMS_TO_TICKS(50));

  // send button released
  hdl->avrc_tl++;
  hdl->avrc_tl &= 0x0F;
  esp_avrc_ct_send_passthrough_cmd(
    hdl->avrc_tl, command, ESP_AVRC_PT_CMD_STATE_RELEASED
  );
}

void handle_track_info_update(
  trackinfoupdate_type_t type, uint8_t *value
)
{
  core_handle_t hdl = get_core_handle();
  trackinfoupdate_t update;
  update.type = type;

  memset(update.value, 0, sizeof(update.value));
  if (value != NULL)
  {
    strncpy(
      (char*) update.value, (const char*) value, sizeof(update.value) - 1
    );
  }

  ESP_LOGI("BT_AVRCP", "Dispatching track info update: type=%d, value='%s'",
           type, (const char *)update.value);

  for (int i = 0; i < CORE_MAX_TRACKINFOUPDATE_CBS; ++i)
  {
    if (hdl->track_info_update_cbs[i] == NULL)
    {
      continue;
    }

    hdl->track_info_update_cbs[i](update, hdl->track_info_update_args[i]);
  }
}

void bt_app_avrc_ct_cb(
  esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param
)
{
  core_handle_t hdl = get_core_handle();
  ESP_LOGI("BT_AVRCP", "AVRCP Callback Fired! Event ID: %d", event);

  uint32_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | 
                       ESP_AVRC_MD_ATTR_ARTIST | 
                       ESP_AVRC_MD_ATTR_ALBUM;

  switch (event)
  {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    {
      ESP_LOGI("BT_AVRCP", "AVRCP connection state changed: connected=%d", param->conn_stat.connected);
      if (param->conn_stat.connected) {
        // ask for the metadata of whatever is currently playing
        ESP_LOGI("BT_AVRCP", "Requesting initial track metadata and subscribing to track change notifications");
        hdl->avrc_tl++;
        hdl->avrc_tl &= 0x0F;
        esp_avrc_ct_send_metadata_cmd(hdl->avrc_tl, attr_mask);

        // subscribe to future track changes
        hdl->avrc_tl++;
        hdl->avrc_tl &= 0x0F;
        esp_avrc_ct_send_register_notification_cmd(
          hdl->avrc_tl, ESP_AVRC_RN_TRACK_CHANGE, 0
        );
      }
      break;
    }
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    {
      ESP_LOGI("BT_AVRCP", "AVRCP change notification: event_id=%d", param->change_ntf.event_id);
      if (param->change_ntf.event_id == ESP_AVRC_RN_TRACK_CHANGE)
      {
        ESP_LOGI("BT_AVRCP", "Track changed, requesting metadata and re-subscribing");
        // ask for the new track's metadata
        hdl->avrc_tl++;
        hdl->avrc_tl &= 0x0F;
        esp_avrc_ct_send_metadata_cmd(hdl->avrc_tl, attr_mask);

        // need to re-subscribe to notifications every time a track changes
        hdl->avrc_tl++;
        hdl->avrc_tl &= 0x0F;
        esp_avrc_ct_send_register_notification_cmd(
          hdl->avrc_tl, ESP_AVRC_RN_TRACK_CHANGE, 0
        );
      }
      else if (param->change_ntf.event_id == ESP_AVRC_RN_PLAY_STATUS_CHANGE)
      {
        uint8_t playback_status = param->change_ntf.event_parameter.playback;
        ESP_LOGI("BT_AVRCP", "Playback status changed: %d", playback_status);

        if (playback_status == ESP_AVRC_PLAYBACK_PLAYING)
        {
          // is_playing = 1;
        }
        else if (playback_status == ESP_AVRC_PLAYBACK_PAUSED || 
                 playback_status == ESP_AVRC_PLAYBACK_STOPPED)
        {
          // is_playing = 0;
        }
      }
      break;
    }
    case ESP_AVRC_CT_METADATA_RSP_EVT:
    {
      uint8_t *attr_text = (uint8_t *)malloc(param->meta_rsp.attr_length + 1);
      if (!attr_text)
      {
        ESP_LOGE("BT_AVRCP", "Failed to allocate memory for metadata response");
        break;
      }
      
      memcpy(attr_text, param->meta_rsp.attr_text, param->meta_rsp.attr_length);
      attr_text[param->meta_rsp.attr_length] = 0;

      ESP_LOGI("BT_AVRCP", "Metadata response: attr_id=0x%lx, len=%d, text='%s'",
               (unsigned long)param->meta_rsp.attr_id,
               param->meta_rsp.attr_length,
               (const char *)attr_text);

      switch (param->meta_rsp.attr_id)
      {
        case ESP_AVRC_MD_ATTR_TITLE:
          handle_track_info_update(TRACKINFOUPDATE_TITLE, attr_text);
          break;
        case ESP_AVRC_MD_ATTR_ARTIST:
          handle_track_info_update(TRACKINFOUPDATE_ARTIST, attr_text);
          break;
      }
      free(attr_text);
      break;
    }

    default:
      ESP_LOGI("BT_AVRCP", "Unhandled AVRCP event: %d", event);
      break;
  }
}
