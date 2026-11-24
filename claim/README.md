# Experimental web claim integration

Before claim, the embedded setup page is derived from `majestic-webui/www/setup.html`.
The three `/eula.en.txt`, `/eula.ru.txt` and `/eula.zh-CN.txt` endpoints serve
the corresponding installed files in `/usr/share/openipc/eula`, not bundled copies.
The firmware Majestic package provides those files. If the English file exists,
`POST /setup` requires exactly one `eula=accepted` and a supported `eula_lang`
(the selected document must be readable). If English is absent the gate is off;
other file errors fail closed. Acceptance must be given by the human owner, not
an agent or automation. A successful claim writes the informational
`/etc/eula-accepted` record as `version language web UTC-time`, like the SSH
claim door. A failed record write is logged but does not undo the password.

`POST /setup` accepts a URL-encoded password of 8–128 bytes and optional matching
confirmation, hashes it with SHA-512 crypt and updates `/etc/shadow` under
`/run/openipc-claim.lock`. Unreadable or malformed shadow fails closed. Once
claimed, setup is forbidden. The service needs permission to read/update shadow.

**Not production-ready.** HTTP exposes the password; use an isolated trusted
network. The claim password does not configure Divinus web/RTSP/ONVIF credentials.
The record is best-effort, not tamper-proof or transactionally coupled to the
shadow update. Clock and installed documents must be trusted; an attacker on the
network can submit a claim first. Uncoordinated shadow writers can race the claim;
never remove the lock file while claimants can run. Cross-toolchain/libcrypt and
full target-device integration remain unverified. Do not deploy this as a security
feature. A crash or directory fsync failure can leave the password set despite an
error. EULA files are expected to be immutable during setup.

The HTTP test binary uses compile-time paths under `/tmp/divinus-http-test` and
never modifies host shadow. It submits **synthetic test consent only**, not a real
owner's agreement. In a disposable Docker container, build and test with:

```
make -C src -j4 OPT='-O2 -pthread' FFMPEG_LIBS=-lm
(cd src && sh ../claim/http_test.sh)
gcc -Wall -Wextra -Werror claim/claim.c claim/test.c -o /tmp/claim-test && /tmp/claim-test
```
