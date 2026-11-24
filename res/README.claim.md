# Experimental x86 console claim adapter

`openipc-claim` is derived from the firmware's `general/overlay/usr/sbin/openipc-claim`.
It is not installed by Divinus or by this repository. It must not be deployed
until the web claim implementation and integration tests are complete.

The adapter holds an exclusive `flock` on `/run/openipc-claim.lock` from the
last shadow check through the `passwd` invocation. The web claimant must acquire
the same lock on the same filesystem, recheck shadow while holding it, and hold
it through the password update and verification. The lock file must never be
removed while either claimant can run. `/run` must be root-owned and not writable
by untrusted users. This only coordinates cooperating claimants; an unmodified
firmware script or an independent password writer does not participate.

The original script and its EULA behavior remain in place. Its initial shadow
check and its general error handling have not yet been hardened. In particular,
the adapter is not a complete security solution and has not been tested on the
target. Do not install it on a camera in its present form.
