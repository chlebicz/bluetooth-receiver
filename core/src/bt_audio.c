#include "bt_internal.h"
#include "a2dp_sink_int_codec_utils.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "BT_AUDIO";

void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len)
{
  static int16_t processed_audio[2048]; // Max A2DP frame is usually around 1024-2048 bytes
  static uint32_t packet_count = 0;

  if (unlikely(len > sizeof(processed_audio))) {
    // Fallback if the packet is unexpectedly large
    ESP_LOGI(TAG, "Dropping oversized audio packet: %lu bytes (max %u)",
             (unsigned long)len, (unsigned)sizeof(processed_audio));

    // bt_a2d_data_hdl(data, len);
    return;
  }

  if (packet_count == 0 || (packet_count % 500) == 0) {
    ESP_LOGI(TAG, "Processing A2DP audio packet #%lu (%lu bytes)",
             (unsigned long)packet_count, (unsigned long)len);
  }
  packet_count++;
  
  int16_t *pcm_data = (int16_t *)data;
  uint32_t num_samples = len / sizeof(int16_t);

  uint32_t stereo_samples = num_samples - (num_samples % 2);

  for (uint32_t i = 0; i < stereo_samples; i += 2)
  {
    int32_t left_sample = pcm_data[i];
    int32_t right_sample = pcm_data[i + 1];

    // average and divide by 2 to prevent clipping
    int32_t result = (left_sample + right_sample) / 4;

    processed_audio[i] = (int16_t)result;
    processed_audio[i + 1] = (int16_t)result;
  }

  if (unlikely(num_samples % 2 != 0)) {
    // We only have 1 slot left, so we just write the Left channel.
    // Because left and right are usually similar in amplitude, 
    // this is our best approximation of the mix for this instant.
    processed_audio[num_samples - 1] = pcm_data[num_samples - 1];
  }

  // Send the modified audio to the I2S sink
  bt_a2d_data_hdl((uint8_t *)processed_audio, len);
}
