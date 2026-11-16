#pragma once
#include "screen/rendering.h"

typedef struct display_handle *display_handle_t;

display_handle_t display_init(void);

void display_start(display_handle_t hdl);

void display_sleep(display_handle_t hdl);

typedef struct {
  typeof(display_init)         *display_init;
  typeof(draw_horizontal_text) *draw_horizontal_text;
  typeof(clear_screen)         *clear_screen;
  typeof(display_start)        *start;
  typeof(display_sleep)        *sleep;
} display_api_t;

const display_api_t *get_display_api(void);
