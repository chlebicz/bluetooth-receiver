#include "power/power.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/gpio.h"

struct power_handle
{
  const core_api_t    *core_api;
  const display_api_t *display_api;

  display_handle_t *display;

  esp_timer_handle_t timer_handle;
};

void power_on(power_handle_t *hdl)
{
  hdl->core_api->start();
  hdl->display_api->start(hdl->display);
}

void power_off(power_handle_t *hdl)
{
  hdl->core_api->sleep();
  hdl->display_api->sleep(hdl->display);

  esp_deep_sleep_start();
}

static void timer_stop(power_handle_t *hdl)
{
  if (hdl->timer_handle != NULL)
  {
    esp_timer_stop(hdl->timer_handle);
    esp_timer_delete(hdl->timer_handle);
    hdl->timer_handle = NULL;
  }
}

static void timer_callback(void *args)
{
  power_handle_t *hdl = (power_handle_t *)args;
  power_off(hdl);
}

static void start_shutdown_timer(power_handle_t *hdl)
{
  timer_stop(hdl);

  const esp_timer_create_args_t timer_args = {
    .callback = &timer_callback,
    .arg      = hdl,
    .name     = "power_timer"
  };

  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &hdl->timer_handle));

  uint64_t timeout_us = 15ULL * 60ULL * 1000000ULL; // 15 minutes

  ESP_ERROR_CHECK(esp_timer_start_once(hdl->timer_handle, timeout_us));
}

static void handle_audio_state_update(core_audio_state_t state, void *args)
{
  power_handle_t *hdl = (power_handle_t *)args;

  if (state == CORE_AUDIO_STATE_PLAYING)
  {
    timer_stop(hdl);
  }
  else
  {
    start_shutdown_timer(hdl);
  }
}

static void handle_connection_state_update(
  core_connection_state_t state,
  void *args
)
{
  power_handle_t *hdl = (power_handle_t *)args;

  if (state == CORE_CONNECTION_STATE_CONNECTED)
  {
    timer_stop(hdl);
  }
  else
  {
    start_shutdown_timer(hdl);
  }
}

power_handle_t *power_init(
  const core_api_t *core_api,
  const display_api_t *display_api,
  display_handle_t *display
)
{
  power_handle_t *hdl = malloc(sizeof(power_handle_t));

  hdl->core_api = core_api;
  hdl->display_api = display_api;

  hdl->display = display;
  hdl->timer_handle = NULL;

  core_api->add_audio_state_change_cb(handle_audio_state_update, hdl);
  core_api->add_connection_state_change_cb(
    handle_connection_state_update,
    hdl
  );

  start_shutdown_timer(hdl);

  return hdl;
}

void power_destroy(power_handle_t *hdl)
{
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
