idf_component_register(
  SRCS "src/power.c"
  INCLUDE_DIRS "include"
  PRIV_REQUIRES driver esp_driver_gpio esp_timer
)
