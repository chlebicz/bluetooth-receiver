#pragma once
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include <stdint.h>

// Common RGB565 Colors
#define COLOR_BLACK   0x0000
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_MAGENTA 0xF81F
#define COLOR_CYAN    0x07FF
#define COLOR_WHITE   0xFFFF

#define LCD_H_RES CONFIG_SCREEN_LCD_H_RES
#define LCD_V_RES CONFIG_SCREEN_LCD_V_RES

typedef struct display_handle display_handle_t;

void clear_screen(display_handle_t *hdl, uint16_t color);

void draw_char(
  display_handle_t *hdl,
  uint8_t c,
  int x,
  int y,
  uint16_t fg_color,
  uint16_t bg_color,
  uint8_t zoom
);

void draw_horizontal_text(
  display_handle_t *hdl,
  const char* str,
  int length,
  int x,
  int y,
  uint16_t fg_color,
  uint16_t bg_color,
  uint8_t zoom
);

void draw_text_band(
  display_handle_t *hdl,
  const char* str,
  int length,
  int x,
  int y,
  uint16_t fg_color,
  uint16_t bg_color,
  uint8_t zoom
);
