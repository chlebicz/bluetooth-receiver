#include "screen/rendering.h"
#include <stdint.h>
#include "esp_heap_caps.h"
#include "screen/font8x8.h"
#include "display_handle.h"

static uint16_t* allocate_pooled_dma_buffer(size_t size)
{
  #define BIG_CHAR_POOL_SIZE 16
  static uint16_t *dma_pool[BIG_CHAR_POOL_SIZE] = {NULL};
  static size_t dma_pool_sizes[BIG_CHAR_POOL_SIZE] = {0};
  static int pool_idx = 0;

  if (dma_pool[pool_idx] != NULL && dma_pool_sizes[pool_idx] < size)
  {
    heap_caps_free(dma_pool[pool_idx]);
    dma_pool[pool_idx] = NULL;
  }

  if (dma_pool[pool_idx] == NULL)
  {
    uint16_t *buffer = heap_caps_malloc(size, MALLOC_CAP_DMA);
    if (buffer != NULL)
    {
      dma_pool[pool_idx] = buffer;
      dma_pool_sizes[pool_idx] = size;
    }
    else
    {
      return NULL;
    }
  }

  uint16_t *buffer_to_return = dma_pool[pool_idx];
  pool_idx = (pool_idx + 1) % BIG_CHAR_POOL_SIZE;

  return buffer_to_return;
}

static inline void draw_zoomed_pixel(
  uint16_t *buffer,
  int band_width,
  int base_x,
  int base_y,
  int zoom,
  uint16_t color
)
{
  for (int r_off = 0; r_off < zoom; r_off++)
  {
    for (int c_off = 0; c_off < zoom; c_off++)
    {
      int out_x = base_x + c_off;

      if (out_x >= 0 && out_x < band_width)
      {
        int out_y = base_y + r_off;
        buffer[out_y * band_width + out_x] = color;
      }
    }
  }
}

static inline void draw_zoomed_char(
  uint16_t *buffer,
  int band_width,
  uint8_t c,
  int char_x,
  int zoom,
  uint16_t color
)
{
  if (c >= 128) c = '?';

  for (int row = 0; row < GLYPH_HEIGHT; row++)
  {
    uint8_t line = font8x8_basic[c][row];

    if (!line)
    {
      continue;
    }

    for (int col = 0; col < GLYPH_WIDTH; col++)
    {
      if (!(line & (1 << col))) {
        continue;
      }

      int base_x = char_x + (col * zoom);
      int base_y = row * zoom;
      draw_zoomed_pixel(buffer, band_width, base_x, base_y, zoom, color);
    }
  }
}

void draw_char(
  display_handle_t hdl,
  uint8_t c,
  int x,
  int y,
  uint16_t fg_color,
  uint16_t bg_color,
  uint8_t zoom
)
{
  esp_lcd_panel_handle_t panel = hdl->panel_handle;

  if (zoom == 0)
  {
    return;
  }

  int scaled_width = GLYPH_WIDTH * zoom;
  int scaled_height = GLYPH_HEIGHT * zoom;
  int total_pixels = scaled_width * scaled_height;

  uint16_t *dma_char_buffer = allocate_pooled_dma_buffer(total_pixels * sizeof(uint16_t));
  if (dma_char_buffer == NULL)
  {
    return;
  }

  uint16_t bg_swapped = __builtin_bswap16(bg_color);
  for (int i = 0; i < total_pixels; i++)
  {
    dma_char_buffer[i] = bg_swapped;
  }

  uint16_t fg_swapped = __builtin_bswap16(fg_color);
  draw_zoomed_char(dma_char_buffer, scaled_width, c, 0, zoom, fg_swapped);

  esp_lcd_panel_draw_bitmap(
    panel, x, y, x + scaled_width, y + scaled_height, dma_char_buffer
  );
}

void draw_horizontal_text(
  display_handle_t hdl,
  const char* str,
  int length,
  int x,
  int y,
  uint16_t fg_color,
  uint16_t bg_color,
  uint8_t zoom
)
{
  if (zoom == 0)
  {
    return;
  }

  for (int i = 0; i < length; i++)
  {
    draw_char(
      hdl,
      (uint8_t)str[i],
      x + (i * GLYPH_SIZE * zoom),
      y,
      fg_color,
      bg_color,
      zoom
    );
  }
}

static uint16_t* prepare_band_buffer(int width, int height, uint16_t bg_color)
{
  static uint16_t *band_buffer = NULL;
  static int allocated_width = 0;
  static int allocated_height = 0;

  if (band_buffer == NULL || width > allocated_width || height > allocated_height)
  {
    if (band_buffer != NULL)
    {
      heap_caps_free(band_buffer);
    }
    band_buffer = heap_caps_malloc(width * height * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (band_buffer == NULL)
    {
      return NULL;
    }
    
    allocated_width = width;
    allocated_height = height;
  }

  uint16_t bg_swapped = __builtin_bswap16(bg_color);
  for (int i = 0; i < width * height; i++)
  {
    band_buffer[i] = bg_swapped;
  }

  return band_buffer;
}

void draw_text_band(
  display_handle_t hdl,
  const char* str,
  int length,
  int scroll_x,
  int y,
  uint16_t fg_color,
  uint16_t bg_color,
  uint8_t zoom
)
{
  esp_lcd_panel_handle_t panel = hdl->panel_handle;

  if (zoom == 0)
  {
    return;
  }

  int band_width = LCD_H_RES;
  int band_height = GLYPH_HEIGHT * zoom;

  uint16_t *band_buffer = prepare_band_buffer(band_width, band_height, bg_color);
  if (!band_buffer)
  {
    return;
  }

  uint16_t fg_swapped = __builtin_bswap16(fg_color);
  for (int i = 0; i < length; i++)
  {
    int char_x = scroll_x + (i * GLYPH_WIDTH * zoom);

    if (char_x + GLYPH_WIDTH * zoom <= 0 || char_x >= band_width)
    {
      continue;
    }

    draw_zoomed_char(band_buffer, band_width, (uint8_t)str[i], char_x, zoom, fg_swapped);
  }

  esp_lcd_panel_draw_bitmap(panel, 0, y, band_width, y + band_height, band_buffer);
}

void clear_screen(display_handle_t hdl, uint16_t color)
{
  esp_lcd_panel_handle_t panel = hdl->panel_handle;

  const int CHUNK_LINES = 20;
  const int PIXELS_PER_CHUNK = LCD_H_RES * CHUNK_LINES;
  
  uint16_t *chunk_buffer = allocate_pooled_dma_buffer(PIXELS_PER_CHUNK * sizeof(uint16_t));
  if (chunk_buffer == NULL)
  {
    return;
  }

  uint16_t swapped_color = __builtin_bswap16(color);
  for (int i = 0; i < PIXELS_PER_CHUNK; i++)
  {
    chunk_buffer[i] = swapped_color;
  }

  for (int y = 0; y < LCD_V_RES; y += CHUNK_LINES)
  {
    int y_end = y + CHUNK_LINES;
    if (y_end > LCD_V_RES)
    {
      y_end = LCD_V_RES;
    }
    
    esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y_end, chunk_buffer);
  }
}