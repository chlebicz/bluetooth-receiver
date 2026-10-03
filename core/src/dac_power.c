#include "driver/gpio.h"

#define DAC_POWER_PIN CONFIG_PIN_CORE_DAC_POWER

void dac_power_init(void)
{
  gpio_set_direction(DAC_POWER_PIN, GPIO_MODE_OUTPUT);
}

void dac_power_on(void)
{
  gpio_set_level(DAC_POWER_PIN, 0);
}

void dac_power_off(void)
{
  gpio_set_level(DAC_POWER_PIN, 1);
}
