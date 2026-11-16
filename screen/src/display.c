#include "screen/display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_ili9341.h"
#include "screen/rendering.h"
#include "display_handle.h"

static const char *TAG = "DISPLAY";

#define PIN_NUM_MISO CONFIG_PIN_SCREEN_MISO
#define PIN_NUM_MOSI CONFIG_PIN_SCREEN_MOSI
#define PIN_NUM_CLK  CONFIG_PIN_SCREEN_CLK
#define PIN_NUM_CS   CONFIG_PIN_SCREEN_CS
#define PIN_NUM_DC   CONFIG_PIN_SCREEN_DC
#define PIN_NUM_RST  CONFIG_PIN_SCREEN_RST
#define PIN_NUM_LED  CONFIG_PIN_SCREEN_LED

const display_api_t *get_display_api(void)
{
  static const display_api_t api = {
    .display_init = display_init,
    .draw_horizontal_text = draw_horizontal_text,
    .clear_screen = clear_screen,
    .start = display_start,
    .sleep = display_sleep
  };

  return &api;
}

display_handle_t display_init(void)
{
  display_handle_t hdl = malloc(sizeof(struct display_handle));
  esp_lcd_panel_handle_t panel_handle = NULL;

  ESP_LOGI(TAG, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
    .sclk_io_num = PIN_NUM_CLK,
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    // Max transfer size: Full screen buffer in bytes
    .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t)
  };
  // We use SPI2_HOST (which is the VSPI hardware on standard ESP32)
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = PIN_NUM_DC,
    .cs_gpio_num = PIN_NUM_CS,
    .pclk_hz = 20 * 1000 * 1000, // 20 MHz SPI clock
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .spi_mode = 3,
    .trans_queue_depth = 10
  };
  ESP_ERROR_CHECK(
    esp_lcd_new_panel_io_spi(
      (esp_lcd_spi_bus_handle_t)SPI2_HOST,
      &io_config,
      &io_handle
    )
  );

  ESP_LOGI(TAG, "Install ILI9341 panel driver");
  esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_NUM_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
    .bits_per_pixel = 16,
    .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE
  };
  
  ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
  hdl->panel_handle = panel_handle;
  hdl->io_handle = io_handle;

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  // Turn the display on
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // Landscape mode
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));

  ESP_LOGI(TAG, "Turn on backlight");
  gpio_set_direction(PIN_NUM_LED, GPIO_MODE_OUTPUT);
  gpio_set_level(PIN_NUM_LED, 1);

  return hdl;
}

void display_start(display_handle_t hdl)
{
  ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(hdl->io_handle, 0x11, NULL, 0)); // Sleep Out
  // vTaskDelay(pdMS_TO_TICKS(120));
  gpio_set_level(PIN_NUM_LED, 1);
}

void display_sleep(display_handle_t hdl)
{
  gpio_set_level(PIN_NUM_LED, 0);
  ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(hdl->io_handle, 0x10, NULL, 0)); // Sleep In
}
