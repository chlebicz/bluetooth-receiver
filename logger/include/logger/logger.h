#pragma once

#include <stdarg.h>

typedef struct logger_handle *logger_handle_t;

typedef struct
{
  logger_handle_t (*init)(void);
  void (*destroy)(logger_handle_t hdl);
  int (*vprintf)(logger_handle_t hdl, const char *fmt, va_list args);
} logger_api_t;

const logger_api_t *uart_logger_get_api(void);
const logger_api_t *debug_uart_logger_get_api(void);
