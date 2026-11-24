# OpenWall settings

`/api/openwall` is a settings-only endpoint. **No sender, scheduler, or upload is implemented**; enabling it does not transmit anything.

The device must be claimed and requests must authenticate as root using HTTP Basic and the shadow password. Cross-site requests are rejected. GET returns JSON with `enabled` (boolean), `interval` (minutes), `caption`, and `camera` (index). Defaults are false, 60, empty, and 0. Query strings are rejected.

POST replaces all settings using `application/x-www-form-urlencoded` and requires the non-simple header `X-OpenWall-Request: 1` as CSRF protection. Fields are `enabled` (0 or 1), `interval` (15, 30, 60 or 120 minutes), `caption` (valid UTF-8, at most 128 bytes, no controls), and `camera` (currently only 0, the local pipeline). Duplicate, missing and unknown fields are rejected. A successful POST returns the new JSON. Clients must use the same origin; no CORS access is provided.

Settings are stored in `/etc/openwall.conf`, atomically replaced with a root-owned mode 0600 file. An invalid, unsafe or unreadable existing file fails closed rather than silently resetting settings. Protect the management connection with a trusted network or TLS termination: Basic authentication alone is not encrypted.
