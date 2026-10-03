#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "esp_attr.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "core/core.h"

#include "screen/display.h"
#include "screen/rendering.h"
#include "app_screen.h"

#include "pins/pins.h"
#include "power/power.h"
#include "buttons.h"

void app_main(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  const core_api_t *core_api = core_get_api();
  core_api->init();

  const display_api_t *display_api = get_display_api();
  display_handle_t *lcd = display_api->display_init();
  display_api->clear_screen(lcd, COLOR_BLACK);

  const power_api_t *power_api = power_get_api();
  power_handle_t *power = power_api->init(core_api, display_api, lcd);
  power_api->power_on(power);

  app_screen_init(core_api, display_api, lcd);

  const pins_api_t *pins_api = pins_get_api();
  pins_handle_t *pins = pins_api->init();

  app_buttons_init(core_api, pins_api, power_api, power, pins);

  while (1)
  {
    vTaskDelay(1000);
  }
}
