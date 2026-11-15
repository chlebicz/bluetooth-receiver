#include "core/core.h"
#include <stddef.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "bredr_app_common_utils.h"
#include "bt_app_core_utils.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "a2dp_sink_common_utils.h"
#include "a2dp_utils_tags.h"
#include "a2dp_sink_int_codec_utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "core/dac_power.h"

#include "core_handle.h"
#include "bt_internal.h"

#define CORE_XSMT_PIN CONFIG_PIN_CORE_XSMT

static const char *TAG = "CORE";

void core_add_track_info_update_cb(
  track_info_update_cb_t cb,
  void *data
)
{
  core_handle_t hdl = get_core_handle();
  bool added = false;
  for (int i = 0; i < CORE_MAX_TRACKINFOUPDATE_CBS; ++i)
  {
    if (hdl->track_info_update_cbs[i] == NULL)
    {
      hdl->track_info_update_cbs[i]  = cb;
      hdl->track_info_update_args[i] = data;
      added = true;
      ESP_LOGI(TAG, "Added track info update callback at slot %d", i);
      break;
    }
  }

  if (!added)
  {
    ESP_LOGE(TAG, "Couldn't add track info update cb - limit exceeded");
  }
}

void core_remove_track_info_update_cb(
  track_info_update_cb_t cb
)
{
  core_handle_t hdl = get_core_handle();
  bool removed = false;
  for (int i = 0; i < CORE_MAX_TRACKINFOUPDATE_CBS; ++i)
  {
    if (hdl->track_info_update_cbs[i] == cb)
    {
      hdl->track_info_update_cbs[i]  = NULL;
      hdl->track_info_update_args[i] = NULL;
      removed = true;
      ESP_LOGI(TAG, "Removed track info update callback from slot %d", i);
      break;
    }
  }

  if (!removed)
  {
    ESP_LOGE(
      TAG, 
      "Couldn't remove track info update cb - provided cb not registered"
    );
  }
}

void core_add_audio_state_change_cb(
  audio_state_update_cb_t cb,
  void *data
)
{
  core_handle_t hdl = get_core_handle();
  bool added = false;
  for (int i = 0; i < CORE_MAX_AUDIO_STATE_CBS; ++i)
  {
    if (hdl->audio_state_update_cbs[i] == NULL)
    {
      hdl->audio_state_update_cbs[i]  = cb;
      hdl->audio_state_update_args[i] = data;
      added = true;
      ESP_LOGI(TAG, "Added audio state change callback at slot %d", i);
      break;
    }
  }

  if (!added)
  {
    ESP_LOGE(TAG, "Couldn't add audio state change cb - limit exceeded");
  }
}

void core_remove_audio_state_change_cb(
  audio_state_update_cb_t cb
)
{
  core_handle_t hdl = get_core_handle();
  bool removed = false;
  for (int i = 0; i < CORE_MAX_AUDIO_STATE_CBS; ++i)
  {
    if (hdl->audio_state_update_cbs[i] == cb)
    {
      hdl->audio_state_update_cbs[i]  = NULL;
      hdl->audio_state_update_args[i] = NULL;
      removed = true;
      ESP_LOGI(TAG, "Removed audio state change callback from slot %d", i);
      break;
    }
  }

  if (!removed)
  {
    ESP_LOGE(
      TAG, 
      "Couldn't remove audio state change cb - provided cb not registered"
    );
  }
}

void core_add_connection_state_change_cb(
  connection_state_update_cb_t cb,
  void *data
)
{
  core_handle_t hdl = get_core_handle();
  bool added = false;
  for (int i = 0; i < CORE_MAX_CONNECTION_STATE_CBS; ++i)
  {
    if (hdl->connection_state_update_cbs[i] == NULL)
    {
      hdl->connection_state_update_cbs[i]  = cb;
      hdl->connection_state_update_args[i] = data;
      added = true;
      ESP_LOGI(TAG, "Added connection state change callback at slot %d", i);
      break;
    }
  }

  if (!added)
  {
    ESP_LOGE(TAG, "Couldn't add connection state change cb - limit exceeded");
  }
}

void core_remove_connection_state_change_cb(
  connection_state_update_cb_t cb
)
{
  core_handle_t hdl = get_core_handle();
  bool removed = false;
  for (int i = 0; i < CORE_MAX_CONNECTION_STATE_CBS; ++i)
  {
    if (hdl->connection_state_update_cbs[i] == cb)
    {
      hdl->connection_state_update_cbs[i]  = NULL;
      hdl->connection_state_update_args[i] = NULL;
      removed = true;
      ESP_LOGI(TAG, "Removed connection state change callback from slot %d", i);
      break;
    }
  }

  if (!removed)
  {
    ESP_LOGE(
      TAG, 
      "Couldn't remove connection state change cb - provided cb not registered"
    );
  }
}

// bluetooth device name
static const char local_device_name[] = CONFIG_EXAMPLE_LOCAL_DEVICE_NAME;

// event for stack up
enum {
  BT_APP_EVT_STACK_UP = 0,
};

static void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
  ESP_LOGI(TAG, "Handling stack event: %d", event);

  switch (event)
  {
  case BT_APP_EVT_STACK_UP:
  {
    ESP_LOGI(TAG, "Bluetooth stack up, configuring GAP, AVRCP, and A2DP");
    print_esp_mac_address();

    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

    ESP_LOGI(TAG, "Setting local Bluetooth device name: '%s'", local_device_name);
    esp_bt_gap_set_device_name(local_device_name);

    esp_bt_cod_t cod;
    cod.major = ESP_BT_COD_MAJOR_DEV_AV;
    cod.minor = 5; // 5 is the official bluetooth hardware class id for loudspeaker
    cod.service = ESP_BT_COD_SRVC_AUDIO | ESP_BT_COD_SRVC_RENDERING;
    ESP_LOGI(TAG, "Setting Bluetooth Class of Device (CoD: major=%d, minor=%d)", cod.major, cod.minor);
    esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_ALL);

    ESP_LOGI(TAG, "Registering BT dev and GAP callbacks");
    esp_bt_dev_register_callback(bt_app_dev_cb);
    esp_bt_gap_register_callback(bt_app_gap_cb);

    ESP_LOGI(TAG, "Initializing AVRCP controller");
    assert(esp_avrc_ct_init() == ESP_OK);

    esp_avrc_ct_register_callback(bt_app_avrc_ct_cb);

    // target - i think this doesn't change anything..
    // assert(esp_avrc_tg_init() == ESP_OK);

    ESP_LOGI(TAG, "Initializing A2DP sink");
    esp_a2d_register_callback(&bt_app_a2d_cb);
    assert(esp_a2d_sink_init() == ESP_OK);

    esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
    // get the default value of the delay value
    esp_a2d_sink_get_delay_value();
    // get local device name
    esp_bt_gap_get_device_name();

    // set discoverable and connectable mode, wait to be connected
    ESP_LOGI(TAG, "Setting scan mode to connectable & general discoverable");
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    break;
  }
  // others
  default:
    ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
    break;
  }
}

bool core_initialised = false;

void core_init(void)
{
  ESP_LOGI(TAG, "Initializing core");
  core_handle_t hdl = get_core_handle();
  if (core_initialised)
  {
    ESP_LOGE(TAG, "there can exist only one instance of core");
    return;
  }

  memset(hdl, 0, sizeof(struct core_handle));

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGI(TAG, "Erasing NVS flash (err=0x%x) and re-initializing", err);
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  esp_reset_reason_t reset_reason = esp_reset_reason();
  ESP_LOGI(TAG, "Reset reason: %d", reset_reason);
  if (reset_reason != ESP_RST_SW) {
    nvs_handle_t nvs_hdl;
    if (nvs_open("storage", NVS_READWRITE, &nvs_hdl) == ESP_OK) {
      ESP_LOGI(TAG, "Non-software reset, resetting wd_restarts counter in NVS");
      nvs_set_u8(nvs_hdl, "wd_restarts", 0);
      nvs_commit(nvs_hdl);
      nvs_close(nvs_hdl);
    }
  }

  ESP_LOGI(TAG, "Initializing BREDR common app");
  ESP_ERROR_CHECK(bredr_app_common_init());

  hdl->a2dp_watchdog_timer = xTimerCreate(
    "a2dp_watchdog", pdMS_TO_TICKS(5000), pdFALSE, hdl, a2dp_watchdog_cb
  );
  ESP_LOGI(TAG, "Created A2DP 5s watchdog timer");

  ESP_LOGI(TAG, "Starting BT app task");
  bt_app_task_start_up();

  // bluetooth device name, connection mode and profile set up
  ESP_LOGI(TAG, "Dispatching BT_APP_EVT_STACK_UP");
  bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

  // vTaskDelay(pdMS_TO_TICKS(2000)); // give the bluetooth stack 2 seconds to fully initialize

  nvs_handle_t nvs_hdl;
  esp_bd_addr_t saved_mac;
  size_t required_size = sizeof(esp_bd_addr_t);
  
  if (nvs_open("storage", NVS_READONLY, &nvs_hdl) == ESP_OK) {
    if (nvs_get_blob(
          nvs_hdl, "last_mac", saved_mac, &required_size
        ) == ESP_OK) {
      ESP_LOGI(TAG, "Found saved MAC in NVS: %02x:%02x:%02x:%02x:%02x:%02x",
               saved_mac[0], saved_mac[1], saved_mac[2],
               saved_mac[3], saved_mac[4], saved_mac[5]);
      // Only auto-connect if the phone hasn't already started connecting to us
      if (!hdl->is_a2dp_connected && !hdl->is_a2dp_connecting) {
        ESP_LOGI(TAG, "auto-connecting to saved device");
        esp_a2d_sink_connect(saved_mac);
      } else {
        ESP_LOGI(TAG, "device already connecting/connected, skipping auto-connect");
      }
    } else {
      ESP_LOGI(TAG, "No saved MAC found in NVS");
    }
    nvs_close(nvs_hdl);
  }

  ESP_LOGI(TAG, "Configuring XSMT mute pin GPIO%d", CORE_XSMT_PIN);
  gpio_set_direction(CORE_XSMT_PIN, GPIO_MODE_OUTPUT);

  dac_power_init();
  dac_power_on();

  core_initialised = true;
  ESP_LOGI(TAG, "Core initialization complete");
}

void core_start(void)
{
  ESP_LOGI(TAG, "Starting core: unmuting XSMT (GPIO%d=1), enabling A2DP clocks, powering on DAC", CORE_XSMT_PIN);
  gpio_set_level(CORE_XSMT_PIN, 1);
  bt_a2d_clocks_enable();
  dac_power_on();
}

void core_sleep(void)
{
  ESP_LOGI(TAG, "Sleeping core: muting XSMT (GPIO%d=0), disabling A2DP clocks, powering off DAC", CORE_XSMT_PIN);
  gpio_set_level(CORE_XSMT_PIN, 0);
  bt_a2d_clocks_disable();
  dac_power_off();
}

void core_disconnect(void)
{
  ESP_LOGI(TAG, "Disconnecting core and clearing saved MAC from NVS");
  nvs_handle_t nvs_hdl;
  if (nvs_open("storage", NVS_READWRITE, &nvs_hdl) == ESP_OK) {
    nvs_erase_key(nvs_hdl, "last_mac");
    nvs_commit(nvs_hdl);
    nvs_close(nvs_hdl);
    ESP_LOGI(TAG, "Erased last_mac from NVS");
  }

  core_handle_t hdl = get_core_handle();
  
  ESP_LOGI(TAG, "Setting scan mode to connectable & general discoverable");
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

  if (!hdl->is_a2dp_connected && !hdl->is_a2dp_connecting)
  {
    ESP_LOGI(TAG, "No active A2DP connection or pending attempt, skipping disconnect");
    return;
  }

  ESP_LOGI(TAG, "Disconnecting A2DP sink from %02x:%02x:%02x:%02x:%02x:%02x",
           hdl->peer_bda[0], hdl->peer_bda[1], hdl->peer_bda[2],
           hdl->peer_bda[3], hdl->peer_bda[4], hdl->peer_bda[5]);
  esp_a2d_sink_disconnect(hdl->peer_bda);
}

const core_api_t *core_get_api()
{
  static const core_api_t api = {
    .init                              = core_init,
    .add_trackinfoupdate_cb            = core_add_track_info_update_cb,
    .remove_track_info_update_cb       = core_remove_track_info_update_cb,
    .add_audio_state_change_cb         = core_add_audio_state_change_cb,
    .remove_audio_state_change_cb      = core_remove_audio_state_change_cb,
    .add_connection_state_change_cb    = core_add_connection_state_change_cb,
    .remove_connection_state_change_cb = core_remove_connection_state_change_cb,
    .start                             = core_start,
    .sleep                             = core_sleep,
    .disconnect                        = core_disconnect
  };

  return &api;
}
