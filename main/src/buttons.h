#pragma once
#include "core/core.h"
#include "pins/pins.h"
#include "power/power.h"

void app_buttons_init(
  const core_api_t *core_api,
  const pins_api_t *pins_api,
  const power_api_t *power_api,
  power_handle_t *power,
  pins_handle_t *pins
);
