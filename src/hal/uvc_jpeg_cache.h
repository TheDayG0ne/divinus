#pragma once

/* Internal helpers: the caller serializes all cache access. */
#include <time.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct uvc_jpeg_cache {
    unsigned char *data;
    size_t length;
    uint64_t captured_us;
};

static void uvc_jpeg_cache_clear(struct uvc_jpeg_cache *cache) {
    free(cache->data);
    memset(cache, 0, sizeof(*cache));
}

/* Reject unknown clock domains, malformed/future times and damaged frames. */
static int uvc_jpeg_cache_update(struct uvc_jpeg_cache *cache,
    const struct v4l2_buffer *b, const unsigned char *data, uint64_t now) {
    uint64_t sec, captured;
    unsigned char *copy;
    if ((b->flags & V4L2_BUF_FLAG_ERROR) ||
        (b->flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) != V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC ||
        b->timestamp.tv_sec < 0 || b->timestamp.tv_usec < 0 ||
        b->timestamp.tv_usec >= 1000000 || !now || !data || b->bytesused < 4 ||
        data[0] != 0xff || data[1] != 0xd8 ||
        data[b->bytesused-2] != 0xff || data[b->bytesused-1] != 0xd9)
        goto reject;
    sec = (uint64_t)b->timestamp.tv_sec;
    if (sec > (UINT64_MAX - (uint64_t)b->timestamp.tv_usec) / 1000000ULL)
        goto reject;
    captured = sec * 1000000ULL + (uint64_t)b->timestamp.tv_usec;
    if (!captured || captured > now) goto reject;
    copy = realloc(cache->data, b->bytesused);
    if (!copy) goto reject;
    cache->data = copy;
    memcpy(copy, data, b->bytesused);
    cache->length = b->bytesused;
    cache->captured_us = captured;
    return 1;
reject:
    uvc_jpeg_cache_clear(cache);
    return 0;
}
