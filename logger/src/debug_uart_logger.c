#include "logger/logger.h"

#include <stdio.h>
#include <stdlib.h>

struct logger_handle
{
  const logger_api_t *uart_api;
  logger_handle_t     uart_hdl;
};

static logger_handle_t debug_uart_logger_init(void)
{
  logger_handle_t hdl = malloc(sizeof(struct logger_handle));
  if (hdl == NULL)
  {
    return NULL;
  }

  hdl->uart_api = uart_logger_get_api();
  hdl->uart_hdl = hdl->uart_api->init();
  if (hdl->uart_hdl == NULL)
  {
    free(hdl);
    return NULL;
  }

  return hdl;
}

static void debug_uart_logger_destroy(logger_handle_t hdl)
{
  if (hdl == NULL)
  {
    return;
  }

  hdl->uart_api->destroy(hdl->uart_hdl);
  free(hdl);
}

static int debug_uart_logger_vprintf(logger_handle_t hdl, const char *fmt, va_list args)
{
  if (hdl != NULL)
  {
    va_list args_copy;
    va_copy(args_copy, args);
    hdl->uart_api->vprintf(hdl->uart_hdl, fmt, args_copy);
    va_end(args_copy);
  }

  return vprintf(fmt, args);
}

const logger_api_t *debug_uart_logger_get_api(void)
{
  static const logger_api_t api = {
    .init    = debug_uart_logger_init,
    .destroy = debug_uart_logger_destroy,
    .vprintf = debug_uart_logger_vprintf,
  };
  return &api;
}
