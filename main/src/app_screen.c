#include "app_screen.h"
#include "screen/rendering.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "screen/font8x8.h"
#include "screen/polish.h"

static const char *TAG = "APP_SCREEN";

typedef struct
{
  const core_api_t    *core_api;
  display_handle_t    display;
  const display_api_t *display_api;
} app_screen_handle_t;

static void app_screen_handle_trackinfoupdate(trackinfoupdate_t update, void *data)
{
  app_screen_handle_t *hdl = (app_screen_handle_t *)data;
  ESP_LOGI(TAG, "Received track info update: type=%d", update.type);

  char* safe_value = strdup((const char*)update.value);
  if (safe_value) replace_polish_characters((uint8_t*)safe_value);
  const char* str_to_draw = safe_value ? safe_value : (const char*)update.value;

  size_t len = strlen(str_to_draw);
  const char* spaces = "                    ";

  const uint8_t zoom = 2;
  const size_t max_chars_in_line = LCD_H_RES / (zoom * GLYPH_SIZE);
  if (len > max_chars_in_line)
  {
    ESP_LOGI(TAG, "Truncating text length from %u to %u chars", (unsigned)len, (unsigned)max_chars_in_line);
    len = max_chars_in_line;
  }

  if (update.type == TRACKINFOUPDATE_ARTIST)
  {
    ESP_LOGI(TAG, "Drawing artist: %s", str_to_draw);
    hdl->display_api->draw_horizontal_text(
      hdl->display,
      spaces,
      strlen(spaces),
      0,
      32,
      COLOR_WHITE,
      COLOR_BLACK,
      zoom
    );
    hdl->display_api->draw_horizontal_text(
      hdl->display,
      str_to_draw,
      len,
      0,
      32,
      COLOR_WHITE,
      COLOR_BLACK,
      zoom
    );
  }
  else if (update.type == TRACKINFOUPDATE_TITLE)
  {
    ESP_LOGI(TAG, "Drawing title: %s", str_to_draw);
    hdl->display_api->draw_horizontal_text(
      hdl->display,
      spaces,
      strlen(spaces),
      0,
      16,
      COLOR_WHITE,
      COLOR_BLACK,
      zoom
    );
    hdl->display_api->draw_horizontal_text(
      hdl->display,
      str_to_draw,
      len,
      0,
      16,
      COLOR_WHITE,
      COLOR_BLACK,
      zoom
    );
  }
  else
  {
    ESP_LOGI(TAG, "Ignoring unhandled track info update type: %d", update.type);
  }

  if (safe_value) free(safe_value);
}

void app_screen_init(
  const core_api_t *core_api,
  const display_api_t *display_api,
  display_handle_t display_handle
)
{
  ESP_LOGI(TAG, "Initializing app screen");
  app_screen_handle_t *hdl = malloc(sizeof(app_screen_handle_t));
  if (!hdl)
  {
    ESP_LOGE(TAG, "Failed to allocate app_screen_handle_t");
    return;
  }

  hdl->core_api    = core_api;
  hdl->display_api = display_api;
  hdl->display     = display_handle;

  ESP_LOGI(TAG, "Registering track info update callback");
  core_api->add_trackinfoupdate_cb(app_screen_handle_trackinfoupdate, hdl);

  char first_line[] = "Now playing:";
  ESP_LOGI(TAG, "Drawing initial header text: '%s'", first_line);
  display_api->draw_horizontal_text(
    display_handle,
    (const char *)first_line,
    strlen((const char*)first_line),
    0,
    0,
    COLOR_RED,
    COLOR_BLACK,
    2
  );
}
