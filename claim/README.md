# Experimental standalone claim component

**Not integrated into Divinus. This component does not protect the camera.**
There is no web setup page, EULA handling, password hashing, authentication,
media gating, RTSP/ONVIF gating, or installation in this change. Do not deploy
it as a security feature. The existing untracked `res/openipc-claim` draft is
not part of this component or installed by it.

`claim_state` reads a shadow file and reports empty, populated/locked, or an
error. Missing, malformed, duplicate, non-regular, or unreadable shadow state
must be treated as an error, not as claimed. This is only a snapshot; it does not
protect an operation against later changes to shadow.

`claim_set_hash` serializes cooperating claimants with a persistent flock file,
checks the root shadow entry under the lock, and atomically replaces shadow only
if its root password field is empty. Locked or populated fields count as already
claimed. Errors fail closed. Callers must supply a properly generated strong crypt
hash and trusted shadow and lock paths; production integration must use
`/etc/shadow` and `/run/openipc-claim.lock`, secure directory ownership, and a
console claimant using the same flock. Never unlink the lock while in use.
The update may have succeeded even when directory fsync reports an error; callers
must recheck state rather than assume a retry is safe. Coordination with unrelated
shadow writers is not provided. Hash algorithm validation and all web/security
policy are responsibilities of future integration.

Run tests in the build image:

```
docker run --rm -v "$PWD:/work" -w /work openipc-x86-divinus:local sh -c 'gcc -Wall -Wextra -Werror claim/claim.c claim/test.c -o /tmp/claim-test && /tmp/claim-test'
```
