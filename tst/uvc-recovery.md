# UVC recovery integration test (Linux x86 server)

Run in an isolated test VM, not on a production camera. Build with `UVC_FFMPEG=1` using the normal project instructions. Use a disposable shadow file/system and network namespace; do not alter production credentials. Enable H.264/MP4 and JPEG, set `UVC_DEVICE` to a stable camera path, and record the process PID.

1. Start with the device absent and the test system unclaimed. Verify setup and static WebUI load, and that RTSP requests and ONVIF discovery are denied before claim. Complete EULA/claim using the normal setup flow; with RTSP authentication configured, verify its credentials are still enforced afterwards.
2. Connect the camera at the configured path without restarting Divinus. Verify H.264 frames arrive and (for MJPEG input) snapshots succeed. Verify the PID is unchanged.
3. Disconnect while streaming and while requesting snapshots. Verify the process and web setup remain responsive, snapshots fail rather than returning the old image, and no RTSP/ONVIF claim restriction changes. Reconnect and verify fresh frames and snapshots resume. Repeat several times, including disconnect during startup and shutdown.
4. Repeat with JPEG enabled and MP4 disabled, then with MP4 enabled and JPEG disabled. Also test an invalid path and a device with unsupported format. Verify retries do not spin or leak descriptors/mappings (`/proc/<pid>/fd`, `/proc/<pid>/maps`).
5. Exercise concurrent snapshot requests and pipeline shutdown/restart under a race detector where available. Confirm no use-after-unmap, deadlock, crash, or stale snapshot. Test YUYV separately: H.264 should recover but JPEG snapshots remain unavailable.

The configured device path must refer to the camera again after reconnection; prefer a stable `/dev/v4l/by-id/` symlink. A device that stops delivering frames without reporting an error is not detected by this test's disconnect mechanism.
