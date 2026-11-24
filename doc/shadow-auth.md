# x86 Linux HTTP authentication

After claim, HTTP uses Basic authentication with username `root` and the current `/etc/shadow` password hash. This is mandatory regardless of `web_enable_auth`, configured web credentials, `web_auth_skiplocal`, or client address. Missing, unreadable, malformed, empty or locked root credentials deny access. The existing unclaimed setup and EULA routes remain available; a missing or unreadable shadow file denies them too. The process needs permission to read shadow. Password changes take effect on subsequent requests.

**ONVIF is explicitly excluded from this HTTP gate** and keeps its independent settings; RTSP is also independent. Neither has been migrated to root authentication. Disable both or restrict their network access until separately secured. Non-x86 platforms retain their previous HTTP authentication behavior.

Basic authentication exposes passwords over plain HTTP. Use an isolated trusted network or a properly secured TLS reverse proxy; this change does not provide TLS, rate limiting, account expiration enforcement, or protection from brute force. Existing open streams may continue after a password change until disconnected.
