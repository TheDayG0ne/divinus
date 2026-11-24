# x86 Linux HTTP and RTSP authentication

After claim, HTTP and every RTSP request use Basic authentication with username `root` and the current `/etc/shadow` password hash. This is mandatory regardless of the configured HTTP or RTSP authentication settings or client address. Missing, unreadable, malformed, empty or locked root credentials deny access. The existing unclaimed setup and EULA routes remain available; a missing or unreadable shadow file denies them too. RTSP connections are refused until claim, and ownership is checked again for each request. The process needs permission to read shadow. Password changes take effect on subsequent requests.

**ONVIF remains explicitly separate** and keeps its independent authentication settings. Restrict its network access or disable it if those settings are inadequate. Non-x86 platforms retain their previous authentication behavior.

Basic authentication exposes passwords over plain HTTP and RTSP. Use an isolated trusted network or appropriately secured transport; this change does not provide TLS, rate limiting, account expiration enforcement, or protection from brute force. Existing open streams may continue after a password change until disconnected.
