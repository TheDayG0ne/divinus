#pragma once

#include "types.h"
#include "globals.h"
#include "tools.h"
#include <pthread.h>

#define UVC_MAX_CHANNELS 8

extern hal_chnstate uvc_state[UVC_MAX_CHANNELS];
extern int (*uvc_vid_cb)(char, hal_vidstream*);

int uvc_hal_init(void);
void uvc_hal_deinit(void);
int uvc_system_init(char *config);
void uvc_system_deinit(void);
int uvc_pipeline_create();
void uvc_pipeline_destroy(void);
int uvc_video_create(char index, hal_vidconfig *config);
int uvc_video_destroy(char index);
int uvc_video_destroy_all(void);
int uvc_channel_create(char index, short width, short height, char framerate, char jpeg);
int uvc_channel_bind(char index);
int uvc_channel_unbind(char index);
int uvc_video_snapshot_grab(char index, hal_jpegdata *jpeg);
void uvc_video_request_idr(char index);
int uvc_channel_grayscale(char enable);
void *uvc_video_thread(void);
int uvc_audio_init(int samplerate);
void uvc_audio_deinit(void);
void *uvc_audio_thread(void);
int uvc_region_create(char handle, hal_rect rect, short opacity);
int uvc_region_setbitmap(char handle, hal_bitmap *bitmap);
void uvc_region_destroy(char handle);
