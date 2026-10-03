#include "core_handle.h"

static struct core_handle s_core = {0};

core_handle_t get_core_handle(void)
{
  return &s_core;
}
