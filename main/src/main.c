#include "esp_log.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "core/core.h"

#include "screen/display.h"
#include "screen/rendering.h"
#include "app_screen.h"

#include "pins/pins.h"
#include "power/power.h"
#include "logger/logger.h"
#include "app_log.h"
#include "buttons.h"

static const char *TAG = "MAIN";

void app_main(void)
{
  const logger_api_t *logger_api = debug_uart_logger_get_api();
  logger_handle_t logger = logger_api->init();
  app_log_init(logger_api, logger);

  ESP_LOGI(TAG, "Starting bluetooth_receiver application");

  ESP_LOGI(TAG, "Initializing NVS flash");
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGI(TAG, "NVS flash needs erase (err=0x%x), erasing and re-initializing", err);
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  ESP_LOGI(TAG, "NVS flash initialized");

  ESP_LOGI(TAG, "Initializing core module");
  const core_api_t *core_api = core_get_api();
  core_api->init();

  ESP_LOGI(TAG, "Initializing display module");
  const display_api_t *display_api = get_display_api();
  display_handle_t lcd = display_api->display_init();
  ESP_LOGI(TAG, "Clearing display screen to black");
  display_api->clear_screen(lcd, COLOR_BLACK);

  ESP_LOGI(TAG, "Initializing power module");
  const power_api_t *power_api = power_get_api();
  power_handle_t power = power_api->init(core_api, display_api, lcd);
  ESP_LOGI(TAG, "Powering on system");
  power_api->power_on(power);

  ESP_LOGI(TAG, "Initializing app screen");
  app_screen_init(core_api, display_api, lcd);

  ESP_LOGI(TAG, "Initializing pins module");
  const pins_api_t *pins_api = pins_get_api();
  pins_handle_t pins = pins_api->init();

  ESP_LOGI(TAG, "Initializing app buttons");
  app_buttons_init(core_api, pins_api, power_api, power, pins);

  ESP_LOGI(TAG, "Initialization complete, entering main loop");
  while (1)
  {
    vTaskDelay(1000);
  }
}
