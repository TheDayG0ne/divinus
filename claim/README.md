# Experimental web claim integration

`GET /setup.html` and `GET /setup` show a password form while root's shadow
password field is empty. `POST /setup` accepts a URL-encoded `password` field,
12–128 bytes, hashes it with SHA-512 crypt and uses `claim_set_hash` with
`/etc/shadow` and `/run/openipc-claim.lock`. Other HTTP endpoints are blocked
until claimed; an unreadable or malformed shadow fails closed. Once claimed,
setup is forbidden. The service must have permission to read and update shadow.

**Not production-ready.** HTTP sends the password in cleartext. EULA display,
acceptance and audit are not implemented: POST is refused when the EULA path
exists or cannot be checked. There is no integration with Divinus web/RTSP/ONVIF
credentials, which are independent configuration values; setting root's password
does not set those credentials. RTSP and other non-HTTP protocols and existing
streams are not gated. The console adapter is not installed and uncoordinated
shadow writers can race with this component. HTTP endpoint integration tests and
target cross-toolchain/libcrypt compatibility still need verification. Do not
deploy this as a security feature.

`claim_state` reports empty, populated/locked, or error. `claim_set_hash`
serializes cooperating claimants using a persistent flock and atomically replaces
shadow only if the root field is empty. It can report an error after the rename
if directory fsync fails. Never remove the lock file while claimants can run.

Run the component test in Docker:

```
docker run --rm -v "$PWD:/work" -w /work openipc-x86-divinus:local sh -c 'gcc -Wall -Wextra -Werror claim/claim.c claim/test.c -o /tmp/claim-test && /tmp/claim-test'
```
