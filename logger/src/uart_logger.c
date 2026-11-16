#include "logger/logger.h"

#include <stdio.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "sdkconfig.h"

#define UART_LOGGER_UART_NUM        ((uart_port_t)CONFIG_UART_LOGGER_UART_NUM)
#define UART_LOGGER_TX_GPIO         ((gpio_num_t)CONFIG_UART_LOGGER_UART_TX_GPIO)
#define UART_LOGGER_BAUD_RATE       CONFIG_UART_LOGGER_UART_BAUD_RATE
#define UART_LOGGER_RX_BUFFER_SIZE  256
#define UART_LOGGER_TX_BUFFER_SIZE  2048
#define UART_LOGGER_MSG_BUFFER_SIZE 512

struct logger_handle
{
  uart_port_t uart_num;
};

static logger_handle_t uart_logger_init(void)
{
  logger_handle_t hdl = malloc(sizeof(struct logger_handle));
  if (hdl == NULL)
  {
    return NULL;
  }

  hdl->uart_num = UART_LOGGER_UART_NUM;

  const uart_config_t uart_config = {
    .baud_rate  = UART_LOGGER_BAUD_RATE,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_param_config(hdl->uart_num, &uart_config));
  ESP_ERROR_CHECK(
    uart_driver_install(
      hdl->uart_num,
      UART_LOGGER_RX_BUFFER_SIZE,
      UART_LOGGER_TX_BUFFER_SIZE,
      0,
      NULL,
      0
    )
  );
  ESP_ERROR_CHECK(
    uart_set_pin(
      hdl->uart_num,
      UART_LOGGER_TX_GPIO,
      UART_PIN_NO_CHANGE,
      UART_PIN_NO_CHANGE,
      UART_PIN_NO_CHANGE
    )
  );

  return hdl;
}

static void uart_logger_destroy(logger_handle_t hdl)
{
  if (hdl == NULL)
  {
    return;
  }

  uart_driver_delete(hdl->uart_num);
  free(hdl);
}

static int uart_logger_vprintf(logger_handle_t hdl, const char *fmt, va_list args)
{
  if (hdl == NULL)
  {
    return 0;
  }

  char buf[UART_LOGGER_MSG_BUFFER_SIZE];
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  if (len <= 0)
  {
    return len;
  }

  size_t write_len = (size_t)len;
  size_t max_write_len = sizeof(buf) - 1;
  if (len > max_write_len) 
  {
    write_len = max_write_len;
  }
  uart_write_bytes(hdl->uart_num, buf, write_len);

  return len;
}

const logger_api_t *uart_logger_get_api(void)
{
  static const logger_api_t api = {
    .init    = uart_logger_init,
    .destroy = uart_logger_destroy,
    .vprintf = uart_logger_vprintf,
  };
  return &api;
}
