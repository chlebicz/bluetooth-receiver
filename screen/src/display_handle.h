#pragma once

#include "esp_lcd_panel_io.h"

struct display_handle
{
  esp_lcd_panel_handle_t panel_handle;
  esp_lcd_panel_io_handle_t io_handle;
};
