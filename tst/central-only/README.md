# central-only Linux regression test

`integration.py` is a real HTTP/process regression test for a **disposable Linux
x86 test VM**. It is intentionally not runnable on the developer PC and does
not compile anything. It requires root because the production claim gate reads
`/etc/shadow` by default; the test harness uses a specially compiled binary
with `-DCLAIM_SHADOW=/tmp/divinus-central-only-test/shadow`
and `-DDIVINUS_STATIC_ROOT=/tmp/divinus-central-only-test/www`
and
`-DDIVINUS_CAMERA_MANIFEST=/tmp/divinus-central-only-test/cameras.manifest`.

Install that test binary as `/tmp/divinus-central-only-test/divinus`, then run:

```sh
sudo ./integration.py --disposable-test-root
```

The test checks the unclaimed gate and then writes a disposable shadow hash
itself: **it does not test POST /setup or an actual claim transition**. It
checks auth ordering, status without HAL fields, cameras, static files,
reserved-path static traps and noncanonical paths, bounded incomplete-header and
body requests, fail-closed API/ONVIF/media routes, no RTSP listener, the SIGHUP exec log and
HTTP recovery, and unchanged config. It does not instrument HAL or prove the
absence of ONVIF UDP discovery. Do not point it at an installed camera or real
shadow file.
