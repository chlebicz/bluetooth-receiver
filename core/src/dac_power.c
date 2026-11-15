#include "driver/gpio.h"
#include "esp_log.h"

#define DAC_POWER_PIN CONFIG_PIN_CORE_DAC_POWER

static const char *TAG = "DAC_POWER";

void dac_power_init(void)
{
  ESP_LOGI(TAG, "Initializing DAC power pin GPIO%d", DAC_POWER_PIN);
  gpio_set_direction(DAC_POWER_PIN, GPIO_MODE_OUTPUT);
}

void dac_power_on(void)
{
  ESP_LOGI(TAG, "Turning DAC power ON (GPIO%d=0)", DAC_POWER_PIN);
  gpio_set_level(DAC_POWER_PIN, 0);
}

void dac_power_off(void)
{
  ESP_LOGI(TAG, "Turning DAC power OFF (GPIO%d=1)", DAC_POWER_PIN);
  gpio_set_level(DAC_POWER_PIN, 1);
}
