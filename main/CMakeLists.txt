idf_component_register(
  SRCS "src/main.c" "src/buttons.c" "src/app_screen.c"
  PRIV_REQUIRES
    driver
    esp_driver_gpio
    esp_driver_i2c
    esp_hw_support
    esp_timer
    nvs_flash
    bt
    bt_app_core_utils
    bredr_app_common_utils
    a2dp_sink_common_utils
    esp_driver_i2s
    esp_ringbuf
    esp_driver_dac
  INCLUDE_DIRS "src"
)