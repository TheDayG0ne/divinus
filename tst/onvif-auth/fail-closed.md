# x86 ONVIF fail-closed test plan

These are integration tests for the future HTTP ONVIF server. They are intentionally kept as test cases only: this change does not build or run them.

Test setup:

- Run an x86 Linux image with ONVIF enabled.
- Configure both `onvif.enable_auth: false` and `onvif.enable_auth: true` in separate runs.
- Use a known `/etc/shadow` root password and generate HTTP Basic credentials for `root`.
- Send a valid SOAP `POST` to `/onvif/device_service`.

Expected cases for **each** auth configuration:

1. No `Authorization` header: HTTP `401`, no SOAP response.
2. `Authorization: Basic` with the configured ONVIF `admin` credentials: HTTP `401`.
3. `Authorization: Basic` with the root username and wrong password: HTTP `401`.
4. `Authorization: Digest ...`: HTTP `401`; no WS-Security/HTTP Digest challenge is advertised.
5. Valid `Authorization: Basic` for `root` according to `/etc/shadow`: request reaches the ONVIF action handler.
6. Missing, unreadable, malformed, empty, or locked root shadow entry: every ONVIF HTTP request is denied (`401`), even if `onvif.enable_auth` is false.
7. A SOAP request containing a valid WS-Security UsernameToken/PasswordDigest but no valid root Basic header: HTTP `401`.
8. A SOAP request containing a valid WS-Security UsernameToken/PasswordDigest together with valid root Basic credentials: it must not be accepted because WSSE Digest is disabled until replay protection and strict XML parsing are implemented.

Also verify that UDP discovery visibility is not treated as authorization: discovery may be answered, but no ONVIF HTTP operation is available without valid root Basic credentials.
