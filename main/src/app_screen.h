#pragma once
#include "core/core.h"
#include "screen/display.h"

void app_screen_init(
  const core_api_t *core_api,
  const display_api_t *display_api,
  display_handle_t *display_handle
);
