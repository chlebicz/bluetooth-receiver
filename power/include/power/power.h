#pragma once
#include "core/core.h"
#include "screen/display.h"

typedef struct power_handle power_handle_t;

power_handle_t *power_init(
  const core_api_t *core_api,
  const display_api_t *display_api,
  display_handle_t *display
);

void power_destroy(power_handle_t *hdl);

void power_on(power_handle_t *hdl);

void power_off(power_handle_t *hdl);

typedef struct
{
  typeof(power_init)    *init;
  typeof(power_destroy) *destroy;
  typeof(power_on)      *power_on;
  typeof(power_off)     *power_off;
} power_api_t;

const power_api_t *power_get_api();
