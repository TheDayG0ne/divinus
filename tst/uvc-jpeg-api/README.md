# Internal UVC fresh-JPEG API tests (Ubuntu)

These tests are intentionally not run by this change (the target camera pipeline and
V4L2 device are required). Run them on Ubuntu in a disposable environment after a
normal project build:

```sh
UVC_DEVICE=/dev/v4l/by-id/<camera> ./tst/uvc-jpeg-api/run.sh
```

The test harness must exercise the following contract through `uvc_jpeg_get()`:

* before the first frame: `UVC_JPEG_NO_FRAME`;
* a frame is returned only as a caller-owned copy, and `uvc_jpeg_frame_release()` is
  safe and clears the descriptor;
* a frame older than `max_age_ms`: `UVC_JPEG_STALE`;
* a frame larger than `max_length`: `UVC_JPEG_TOO_LARGE`;
* concurrent callers receive independent copies (no lock held after return);
* unplug clears the cached frame and returns `NO_FRAME`/failure, never the prior JPEG;
* reconnect resumes without opening another camera (the existing pipeline is reused);
* a YUYV-only device returns `UVC_JPEG_UNSUPPORTED`;
* shutdown/restart and unplug during capture do not deadlock or expose unmapped data.

The script is a deliberately non-invasive Ubuntu smoke-test wrapper. It does not
start a second capture pipeline or provide an HTTP/sender path.
