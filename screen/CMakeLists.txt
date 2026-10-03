idf_component_register(
  SRCS
    "src/display.c"
    "src/font8x8.c"
    "src/polish.c"
    "src/rendering.c"
  INCLUDE_DIRS "include"
  REQUIRES driver esp_lcd esp_lcd_ili9341
)
