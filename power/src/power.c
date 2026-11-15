#include "power/power.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

static const char *TAG = "POWER";

struct power_handle
{
  const core_api_t    *core_api;
  const display_api_t *display_api;

  display_handle_t display;

  esp_timer_handle_t timer_handle;
  int is_connected;
  int is_playing;
};

void power_on(power_handle_t hdl)
{
  ESP_LOGI(TAG, "Powering on core and display");
  hdl->core_api->start();
  hdl->display_api->start(hdl->display);
}

void power_off(power_handle_t hdl)
{
  ESP_LOGI(TAG, "Powering off: putting core and display to sleep");
  hdl->core_api->sleep();
  hdl->display_api->sleep(hdl->display);

  ESP_LOGI(TAG, "Entering deep sleep");
  esp_deep_sleep_start();
}

static void timer_stop(power_handle_t hdl)
{
  if (hdl->timer_handle != NULL)
  {
    ESP_LOGI(TAG, "Stopping and deleting auto-shutdown timer");
    esp_timer_stop(hdl->timer_handle);
    esp_timer_delete(hdl->timer_handle);
    hdl->timer_handle = NULL;
  }
}

static void timer_callback(void *args)
{
  ESP_LOGI(TAG, "Auto-shutdown timer expired, initiating power off");
  power_handle_t hdl = (power_handle_t)args;
  power_off(hdl);
}

static void start_shutdown_timer(power_handle_t hdl)
{
  timer_stop(hdl);

  const esp_timer_create_args_t timer_args = {
    .callback = &timer_callback,
    .arg      = hdl,
    .name     = "power_timer"
  };

  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &hdl->timer_handle));

  uint64_t timeout_us = 15ULL * 60ULL * 1000000ULL; // 15 minutes

  ESP_LOGI(TAG, "Starting auto-shutdown timer (%llu seconds)", (unsigned long long)(timeout_us / 1000000ULL));
  ESP_ERROR_CHECK(esp_timer_start_once(hdl->timer_handle, timeout_us));
}

static void update_timer_state(power_handle_t hdl)
{
  ESP_LOGI(TAG, "Updating shutdown timer state: is_connected=%d, is_playing=%d", hdl->is_connected, hdl->is_playing);
  if (hdl->is_connected && hdl->is_playing)
  {
    ESP_LOGI(TAG, "Active playback detected, keeping device awake");
    timer_stop(hdl);
  }
  else
  {
    ESP_LOGI(TAG, "Idle or disconnected, arming shutdown timer");
    start_shutdown_timer(hdl);
  }
}

static void handle_audio_state_update(core_audio_state_t state, void *args)
{
  power_handle_t hdl = (power_handle_t)args;
  hdl->is_playing = (state == CORE_AUDIO_STATE_PLAYING);
  ESP_LOGI(TAG, "Audio state update: state=%d, is_playing=%d", state, hdl->is_playing);
  update_timer_state(hdl);
}

static void handle_connection_state_update(
  core_connection_state_t state,
  void *args
)
{
  power_handle_t hdl = (power_handle_t)args;
  hdl->is_connected = (state == CORE_CONNECTION_STATE_CONNECTED);
  ESP_LOGI(TAG, "Connection state update: state=%d, is_connected=%d", state, hdl->is_connected);
  update_timer_state(hdl);
}

power_handle_t power_init(
  const core_api_t *core_api,
  const display_api_t *display_api,
  display_handle_t display
)
{
  ESP_LOGI(TAG, "Initializing power manager");
  power_handle_t hdl = malloc(sizeof(struct power_handle));

  hdl->core_api = core_api;
  hdl->display_api = display_api;

  hdl->display = display;
  hdl->timer_handle = NULL;
  hdl->is_connected = 0;
  hdl->is_playing = 0;

  ESP_LOGI(TAG, "Registering audio and connection state callbacks with core");
  core_api->add_audio_state_change_cb(handle_audio_state_update, hdl);
  core_api->add_connection_state_change_cb(
    handle_connection_state_update,
    hdl
  );

  start_shutdown_timer(hdl);

  return hdl;
}

void power_destroy(power_handle_t hdl)
{
  ESP_LOGI(TAG, "Destroying power manager");
  timer_stop(hdl);
  hdl->core_api->remove_audio_state_change_cb(handle_audio_state_update);
  hdl->core_api->remove_connection_state_change_cb(
    handle_connection_state_update
  );
  free(hdl);
}

const power_api_t *power_get_api()
{
  static const power_api_t api = {
    .init      = power_init,
    .destroy   = power_destroy,
    .power_on  = power_on,
    .power_off = power_off
  };
  return &api;
}
