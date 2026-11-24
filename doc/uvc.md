# UVC backend (x86)

Set `UVC_DEVICE` to the V4L2 capture device (default `/dev/video0`) and build with `UVC_FFMPEG=1`. The backend negotiates MJPEG or YUYV input and encodes H.264; H.265 and audio are not supported. The H.264 stream requires MP4 to be enabled for the HTTP H.26x and MP4 endpoints.

JPEG snapshots use the latest captured MJPEG frame. They fail until a frame has arrived and are unavailable when the camera negotiates YUYV: no JPEG encoding is performed for YUYV. Snapshot width, height, quality and grayscale parameters do not modify the captured frame. The continuous MJPEG output is not supported by this backend; enabling it fails. UVC capture starts when an H.264 channel is bound or JPEG is enabled, including JPEG-only configurations.

The web and setup services can start without the camera. When capture is requested, the backend retries the configured device once per second after an open or streaming failure; use a stable `/dev/v4l/by-id/` path so it resolves again after reconnection. Snapshots fail until a fresh MJPEG frame arrives. A camera that silently stops producing frames without reporting an error is not automatically reopened.

The backend currently uses the negotiated camera dimensions and fixed encoder settings rather than the configured MP4 dimensions, bitrate, frame rate and GOP. IDR requests and grayscale control are not implemented. OSD is not supported.
