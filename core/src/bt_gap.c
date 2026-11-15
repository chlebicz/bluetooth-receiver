#include "bt_internal.h"
#include "bredr_app_common_utils.h"
#include "esp_log.h"
#include "nvs.h"
#include <string.h>

void bt_app_dev_cb(
  esp_bt_dev_cb_event_t event, esp_bt_dev_cb_param_t *param
)
{
  bredr_app_dev_evt_def_hdl(event, param);
}

void a2dp_watchdog_cb(TimerHandle_t xTimer)
{
  core_handle_t hdl = (core_handle_t) pvTimerGetTimerID(xTimer);
  ESP_LOGE(
    "BT_WATCHDOG", 
    "A2DP connection hasn't started after 5s. Rebooting to reopen it."
  );
  
  nvs_handle_t nvs_hdl;
  if (nvs_open("storage", NVS_READWRITE, &nvs_hdl) == ESP_OK) {
    uint8_t wd_restarts = 0;
    nvs_get_u8(nvs_hdl, "wd_restarts", &wd_restarts);
    
    if (wd_restarts < 2) {
      wd_restarts++;
      nvs_set_u8(nvs_hdl, "wd_restarts", wd_restarts);
      nvs_set_blob(nvs_hdl, "last_mac", hdl->current_acl_bda, 
                   sizeof(esp_bd_addr_t));
      nvs_commit(nvs_hdl);
      nvs_close(nvs_hdl);
      
      vTaskDelay(pdMS_TO_TICKS(500));
      esp_restart();
    } else {
      ESP_LOGE(
        "BT_WATCHDOG", 
        "Reached limit of 2 consecutive watchdog restarts. Giving up."
      );
      nvs_set_u8(nvs_hdl, "wd_restarts", 0);
      nvs_commit(nvs_hdl);
      nvs_close(nvs_hdl);

      esp_a2d_sink_disconnect(hdl->current_acl_bda);
    }
  }
}

bool is_device_bonded(esp_bd_addr_t bda) {
  int dev_num = esp_bt_gap_get_bond_device_num();
  if (dev_num <= 0)
  {
    return false;
  }

  esp_bd_addr_t *dev_list = malloc(sizeof(esp_bd_addr_t) * dev_num);
  if (!dev_list)
  {
    return false;
  }

  esp_bt_gap_get_bond_device_list(&dev_num, dev_list);

  bool found = false;
  for (int i = 0; i < dev_num; i++)
  {
    if (memcmp(bda, dev_list[i], sizeof(esp_bd_addr_t)) == 0)
    {
      found = true;
      break;
    }
  }

  free(dev_list);

  return found;
}

void bt_app_gap_cb(
  esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param
)
{
  core_handle_t hdl = get_core_handle();
  switch (event) {
  // this event triggers when the remote name has been successfully fetched
  case ESP_BT_GAP_READ_REMOTE_NAME_EVT:
  {
    if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS)
    {
      // ESP_LOGI("BT_GAP", ">>> Connected Device Name: %s <<<", param->read_rmt_name.rmt_name);
    } else {
      ESP_LOGW("BT_GAP", "Failed to read remote device name");
    }
    break;
  }
  case ESP_BT_GAP_AUTH_CMPL_EVT:
  {
    if (param->auth_cmpl.stat != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE("BT_GAP", "Authentication failed! Removing bond.");
      esp_bt_gap_remove_bond_device(param->auth_cmpl.bda);
      esp_a2d_sink_disconnect(param->auth_cmpl.bda);
      esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    }
    bredr_app_gap_evt_def_hdl(event, param);
    break;
  }
  case ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
  {
    bredr_app_gap_evt_def_hdl(event, param);
    // DON'T set NON_CONNECTABLE here — this fires when the raw ACL link
    // establishes, BEFORE A2DP has a chance to connect on top of it.
    // The A2DP connection handler (in a2dp_sink_int_codec_utils.c) already
    // sets NON_CONNECTABLE when A2DP actually connects successfully.
    
    // Start watchdog timer ONLY if this is a reconnection (device is bonded)
    // If it's a first pairing, we don't want to rush the AVDTP connection because
    // the user needs time to confirm the PIN on their phone/PC.
    memcpy(
      hdl->current_acl_bda, 
      param->acl_conn_cmpl_stat.bda, 
      sizeof(esp_bd_addr_t)
    );
    if (hdl->a2dp_watchdog_timer && is_device_bonded(hdl->current_acl_bda)) {
      if (!hdl->is_a2dp_connected) {
        ESP_LOGI("BT_GAP", "Device is bonded. Starting 5s diagnostic watchdog.");
        xTimerStart(hdl->a2dp_watchdog_timer, 0);
      }
    } else {
      ESP_LOGI("BT_GAP", "Device is NOT bonded (first pairing). Waiting patiently.");
    }
    break;
  }
  case ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
  {
    ESP_LOGI("BT_GAP", "Radio link fully severed. Safe to open doors.");
    if (hdl->a2dp_watchdog_timer) {
      xTimerStop(hdl->a2dp_watchdog_timer, 0);
    }
    
    // Force the A2DP/AVDTP state machine to clean up for this specific device.
    // If the device drops the ACL link without cleanly closing the AVDTP channel,
    // the ESP32's internal state machine gets stuck thinking the channel is "busy".
    esp_a2d_sink_disconnect(param->acl_disconn_cmpl_stat.bda);
    
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    
    bredr_app_gap_evt_def_hdl(event, param);
    break;
  }
  default:
    bredr_app_gap_evt_def_hdl(event, param);
    break;
  }
}

void print_esp_mac_address(void)
{
  const uint8_t *mac = esp_bt_dev_get_address();
  
  if (mac != NULL) {
    ESP_LOGI(
      "MAC_INFO", "My ESP32 Bluetooth MAC Address is: %02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
  } else {
    ESP_LOGW("MAC_INFO", "Bluetooth stack is not initialized yet!");
  }
}
