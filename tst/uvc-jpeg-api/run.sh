#!/bin/sh
set -eu

: "${UVC_DEVICE:?set UVC_DEVICE to the existing pipeline's camera path}"
case "$UVC_DEVICE" in
  /dev/*) ;;
  *) echo "UVC_DEVICE must be a V4L2 /dev path" >&2; exit 2 ;;
esac

echo "Manual Ubuntu test plan for uvc_jpeg_get(max_age_ms, max_length, out):"
echo "  1. Start the normal pipeline with UVC_DEVICE=$UVC_DEVICE."
echo "  2. Call before a frame: expect UVC_JPEG_NO_FRAME."
echo "  3. Call with finite age/length limits: expect a copied MJPEG frame."
echo "  4. Release it; verify independent concurrent copies and no stale frame."
echo "  5. Unplug/reconnect the same device; expect failure while absent and fresh data after reconnect."
echo "  6. Repeat with YUYV-only input: expect UVC_JPEG_UNSUPPORTED."
