#include "app_log.h"

#include <stdarg.h>
#include <stddef.h>

#include "esp_log.h"

static const logger_api_t *s_logger_api = NULL;
static logger_handle_t s_logger = NULL;

static int app_log_vprintf(const char *fmt, va_list args)
{
  return s_logger_api->vprintf(s_logger, fmt, args);
}

void app_log_init(
  const logger_api_t *logger_api,
  logger_handle_t logger
)
{
  s_logger_api = logger_api;
  s_logger = logger;
  esp_log_set_vprintf(app_log_vprintf);
}
