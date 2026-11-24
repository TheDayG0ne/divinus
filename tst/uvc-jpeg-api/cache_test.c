#include <assert.h>
#include <stdio.h>
#include "uvc_jpeg_cache.h"

int main(void) {
    struct uvc_jpeg_cache c = {0};
    struct v4l2_buffer b = {0};
    unsigned char jpeg[] = {0xff, 0xd8, 1, 2, 0xff, 0xd9};
    b.flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
    b.timestamp.tv_sec = 1;
    b.timestamp.tv_usec = 20;
    b.bytesused = sizeof(jpeg);
    assert(uvc_jpeg_cache_update(&c, &b, jpeg, 2000000));
    assert(c.captured_us == 1000020 && c.length == sizeof(jpeg));
    jpeg[2] = 3;
    assert(c.data[2] == 1);
    b.bytesused = 0;
    assert(!uvc_jpeg_cache_update(&c, &b, jpeg, 2000000));
    assert(!c.data && !c.length && !c.captured_us);
    b.bytesused = sizeof(jpeg);
    assert(uvc_jpeg_cache_update(&c, &b, jpeg, 2000000));
    b.flags |= V4L2_BUF_FLAG_ERROR;
    assert(!uvc_jpeg_cache_update(&c, &b, jpeg, 2000000));
    b.flags = 0;
    assert(!uvc_jpeg_cache_update(&c, &b, jpeg, 2000000));
    b.flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
    assert(!uvc_jpeg_cache_update(&c, &b, jpeg, 1000019));
    b.timestamp.tv_usec = 1000000;
    assert(!uvc_jpeg_cache_update(&c, &b, jpeg, 3000000));
    b.timestamp.tv_usec = 20;
    jpeg[0] = 0;
    assert(!uvc_jpeg_cache_update(&c, &b, jpeg, 2000000));
    jpeg[0] = 0xff; jpeg[5] = 0;
    assert(!uvc_jpeg_cache_update(&c, &b, jpeg, 2000000));
    uvc_jpeg_cache_clear(&c);
    puts("uvc jpeg cache tests passed");
    return 0;
}
