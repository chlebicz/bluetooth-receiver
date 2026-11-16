#pragma once

#include "logger/logger.h"

void app_log_init(
  const logger_api_t *logger_api,
  logger_handle_t logger
);
