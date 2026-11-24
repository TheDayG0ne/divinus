# UVC JPEG cache tests

Run `./test.sh uvc-jpeg-api` or `./tst/uvc-jpeg-api/run.sh` on Linux with a C compiler and V4L2 headers. The standalone script can be invoked from any working directory.
These automatic unit tests need no camera. They cover cache ownership, empty and
error buffers, JPEG markers, and timestamp validation. They do not test device
I/O, public API concurrency, or unplug/reconnect: exercise those on a real camera.

Disconnect is detected asynchronously. Until poll or an ioctl reports failure,
the cache can still be returned within the caller's age limit; a silent camera
is not detected as unplugged. The shared mutex covers poll (250 ms), device
ioctls, encoding and memory copies, so API latency has no strict upper bound.
