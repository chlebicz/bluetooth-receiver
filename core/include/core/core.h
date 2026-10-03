#pragma once
#include "trackinfoupdate.h"

void core_init(void);

void core_start(void);

void core_sleep(void);

void core_disconnect(void);

typedef void (*track_info_update_cb_t)(trackinfoupdate_t info, void *data);

void core_add_track_info_update_cb(
  track_info_update_cb_t cb,
  void *data
);

void core_remove_track_info_update_cb(
  track_info_update_cb_t cb
);

typedef enum
{
  CORE_AUDIO_STATE_PLAYING,
  CORE_AUDIO_STATE_PAUSED
} core_audio_state_t;

typedef void (*audio_state_update_cb_t)(
  core_audio_state_t audio_state,
  void *data
);

void core_add_audio_state_change_cb(
  audio_state_update_cb_t cb,
  void *data
);

void core_remove_audio_state_change_cb(
  audio_state_update_cb_t cb
);

typedef enum
{
  CORE_CONNECTION_STATE_CONNECTED,
  CORE_CONNECTION_STATE_DISCONNECTED
} core_connection_state_t;

typedef void (*connection_state_update_cb_t)(
  core_connection_state_t connection_state,
  void *data
);

void core_add_connection_state_change_cb(
  connection_state_update_cb_t cb,
  void *data
);

void core_remove_connection_state_change_cb(
  connection_state_update_cb_t cb
);

typedef struct {
  typeof(core_init)                              *init;
  typeof(core_add_track_info_update_cb)          *add_trackinfoupdate_cb;
  typeof(core_remove_track_info_update_cb)       *remove_track_info_update_cb;
  typeof(core_add_audio_state_change_cb)         *add_audio_state_change_cb;
  typeof(core_remove_audio_state_change_cb)      *remove_audio_state_change_cb;
  typeof(core_add_connection_state_change_cb)    *add_connection_state_change_cb;
  typeof(core_remove_connection_state_change_cb) *remove_connection_state_change_cb;
  typeof(core_start)                             *start;
  typeof(core_sleep)                             *sleep;
  typeof(core_disconnect)                        *disconnect;
} core_api_t;

const core_api_t *core_get_api();
