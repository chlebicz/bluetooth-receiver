idf_component_register(
  SRCS "src/uart_logger.c" "src/debug_uart_logger.c"
  INCLUDE_DIRS "include"
  PRIV_REQUIRES driver esp_driver_uart esp_driver_gpio
)
