idf_component_register(
  SRCS 
    "src/core.c"
    "src/core_handle.c"
    "src/bt_gap.c"
    "src/bt_a2dp.c"
    "src/bt_avrcp.c"
    "src/bt_audio.c"
    "src/dac_power.c"

  PRIV_REQUIRES
    nvs_flash
    bt
    bt_app_core_utils
    bredr_app_common_utils
    a2dp_sink_common_utils
    esp_ringbuf
    driver
    esp_driver_gpio

  INCLUDE_DIRS "include"
)
