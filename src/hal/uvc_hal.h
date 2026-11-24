#pragma once

#include "types.h"
#include "globals.h"
#include "tools.h"
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#define UVC_MAX_CHANNELS 8

/* Caller-owned copy of a fresh MJPEG frame from the existing pipeline. */
typedef struct {
    unsigned char *data;
    size_t length;
    uint64_t captured_monotonic_us;
} uvc_jpeg_frame;

typedef enum {
    UVC_JPEG_OK = 0,
    UVC_JPEG_INVALID = -1,
    UVC_JPEG_NO_FRAME = -2,
    UVC_JPEG_STALE = -3,
    UVC_JPEG_TOO_LARGE = -4,
    UVC_JPEG_UNSUPPORTED = -5
} uvc_jpeg_result;

/* out must point to a zero-initialized or released descriptor. On success it
 * owns data until release; release before reuse. On failure out is unchanged.
 * Limits must be nonzero. Calls serialize with capture, including poll (up to
 * 250 ms), device ioctls, encoding and copies; there is no latency bound.
 * Disconnect invalidates the cache only after the capture thread detects it;
 * until then age is the only freshness bound. */
uvc_jpeg_result uvc_jpeg_get(unsigned int max_age_ms, size_t max_length,
    uvc_jpeg_frame *out);
void uvc_jpeg_frame_release(uvc_jpeg_frame *frame);

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
