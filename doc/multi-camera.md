# Camera registry: launcher manifest

`GET /api/cameras` returns a version 1 read-only inventory from the central Divinus process. The launcher atomically publishes `/run/divinus/cameras.manifest`, a root-owned mode-0600 regular file containing at most four lines in the exact form `slot=N rtsp_port=P`. `N` is 1..4 and `P` is 1..65535. The producer must validate configured slots and ports before publishing; a missing, malformed, or untrusted manifest yields HTTP 503. An empty manifest is valid and represents zero configured slots. Clients cannot select a device path or URL.

Each entry contains `id` (`camera-N`) and the configured `rtsp_port`. `status` is always `unknown`: this process has no reliable per-worker liveness proof, and neither `/dev` presence nor a listening port is treated as streaming. No snapshot, MJPEG, MP4, or same-origin route is advertised because those routes are not implemented for independent worker slots. RTSP URLs are intentionally not synthesized: the worker address and RTSP path contract is not owned by this endpoint. Existing single-camera URLs remain unchanged.

The endpoint is read-only, rejects queries and POSTs, sends `Cache-Control: no-store`, and runs after the existing claim, HTTP authentication and CSRF checks. Authentication policy remains platform-specific, exactly as for other HTTP endpoints. Do not expose it as a public inventory on installations where HTTP authentication is disabled.

## Next architectural blocker

This is not multi-camera discovery or a stream mux. The media pipeline, UVC capture/encoder, JPEG state and HTTP subscribers currently contain process-wide state and do not identify a source. Implement isolated per-camera lifecycle and frame/encoder state, stable identities across reconnects, synchronized status and per-camera subscriber routing before advertising additional cameras or per-camera routes. Discovery must be restricted to administrator-configured local devices, resolve identity safely across hotplug, avoid opening arbitrary client-supplied paths or URLs, and distinguish inaccessible, disconnected and live devices. It must not leak credentials or provide SSRF. Apply the existing claim/auth/CSRF gates to every new route; define capability semantics from actual backend support and configuration, and test reconnection, concurrent clients and isolation on Linux with real or simulated devices before enabling it.

The HTTP integration regression lives in `claim/http_test.py`; run `claim/http_test.sh` in `src` after a Linux native build, as documented in that script.
