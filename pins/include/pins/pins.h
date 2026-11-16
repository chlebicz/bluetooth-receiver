#pragma once

#include "driver/gpio.h"

typedef struct pins_handle *pins_handle_t;

typedef struct
{
  gpio_num_t gpio_pin_num;
  void (*interrupt_callback)(void *args);
  void *args;
} pins_input_pin_t;

void pins_setup_input_pin(pins_handle_t hdl, pins_input_pin_t *pin);
void pins_setup_input_pins(pins_handle_t hdl, size_t length, pins_input_pin_t *pins);

void pins_remove_input_pin(pins_handle_t hdl, gpio_num_t pin_num);
void pins_remove_input_pins(pins_handle_t hdl, size_t length, gpio_num_t *pin_nums);
void pins_remove_all_input_pins(pins_handle_t hdl);

pins_handle_t pins_init(void);

void pins_destroy(pins_handle_t hdl);

void pins_setup_wakeup_pin(pins_input_pin_t *pin);

typedef struct {
  typeof(pins_init)                  *init;
  typeof(pins_destroy)               *destroy;
  typeof(pins_setup_input_pin)       *setup_input_pin;
  typeof(pins_setup_input_pins)      *setup_input_pins;
  typeof(pins_remove_input_pin)      *remove_input_pin;
  typeof(pins_remove_input_pins)     *remove_input_pins;
  typeof(pins_remove_all_input_pins) *remove_all_input_pins;
  typeof(pins_setup_wakeup_pin)      *setup_wakeup_pin;
} pins_api_t;

const pins_api_t *pins_get_api(void);
