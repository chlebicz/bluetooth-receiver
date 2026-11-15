#include "buttons.h"
#include <stdlib.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUTTONS";

typedef struct
{
  const core_api_t  *core_api;
  const power_api_t *power_api;
  power_handle_t    power;
} app_buttons_handle_t;

static void handle_power_button_press(void *args)
{
  ESP_LOGI(TAG, "Power button pressed, initiating power off");
  app_buttons_handle_t *hdl = (app_buttons_handle_t *)args;
  hdl->power_api->power_off(hdl->power);
}

static void handle_bt_button_press(void *args)
{
  ESP_LOGI(TAG, "Bluetooth button pressed, disconnecting current device");
  app_buttons_handle_t *hdl = (app_buttons_handle_t *)args;
  hdl->core_api->disconnect();
}

void app_buttons_init(
  const core_api_t *core_api,
  const pins_api_t *pins_api,
  const power_api_t *power_api,
  power_handle_t power,
  pins_handle_t pins
)
{
  ESP_LOGI(TAG, "Initializing application buttons");
  app_buttons_handle_t *hdl = malloc(sizeof(app_buttons_handle_t));
  hdl->core_api = core_api;
  hdl->power_api = power_api;
  hdl->power = power;

  const gpio_num_t power_button_pin = CONFIG_PIN_BUTTON_POWER;
  const gpio_num_t bt_button_pin    = CONFIG_PIN_BUTTON_BT;
  ESP_LOGI(TAG, "Configured button pins: power=GPIO%d, bt=GPIO%d", power_button_pin, bt_button_pin);

  gpio_reset_pin(power_button_pin);
  gpio_set_direction(power_button_pin, GPIO_MODE_INPUT);
  uint32_t wakeup_causes = esp_sleep_get_wakeup_causes();
  ESP_LOGI(TAG, "Wakeup causes bitmask: 0x%lx", (unsigned long)wakeup_causes);
  if (wakeup_causes & BIT(ESP_SLEEP_WAKEUP_EXT0))
  {
    ESP_LOGI(TAG, "Woke up via EXT0 (power button), waiting for button release");
    while (gpio_get_level(power_button_pin) == 0)
    {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG, "Power button released");
  }

  pins_input_pin_t power_button = {
    .gpio_pin_num = power_button_pin,
    .interrupt_callback = handle_power_button_press,
    .args = hdl
  };
  pins_input_pin_t bt_button = {
    .gpio_pin_num = bt_button_pin,
    .interrupt_callback = handle_bt_button_press,
    .args = hdl
  };

  pins_input_pin_t input_pins[] = {power_button, bt_button};

  ESP_LOGI(TAG, "Registering %d input button pins", (int)(sizeof(input_pins) / sizeof(pins_input_pin_t)));
  pins_api->setup_input_pins(
    pins,
    sizeof(input_pins) / sizeof(pins_input_pin_t),
    input_pins
  );

  rtc_gpio_pullup_en(power_button_pin);
  rtc_gpio_pulldown_dis(power_button_pin);

  ESP_LOGI(TAG, "Enabling EXT0 wakeup on GPIO%d (level 0)", power_button_pin);
  esp_sleep_enable_ext0_wakeup(power_button_pin, 0);
}
