#pragma once
#include <stdint.h>

typedef enum {
  TRACKINFOUPDATE_TITLE,
  TRACKINFOUPDATE_ARTIST
} trackinfoupdate_type_t;

#define MAX_TRACKINFOUPDATE_STRLEN CONFIG_CORE_MAX_TRACKINFOUPDATE_STRLEN

typedef struct
{
  trackinfoupdate_type_t type;
  uint8_t value[MAX_TRACKINFOUPDATE_STRLEN];
} trackinfoupdate_t;
