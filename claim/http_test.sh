#!/bin/sh
set -eu
root=${TMPDIR:-/tmp}/divinus-http-test
rm -rf "$root"
mkdir -p "$root/eula"
printf 'root::1:2:3:4:5:6:7\n' > "$root/shadow"
printf '# Majestic Software End-User License Agreement\nVersion 1.0, 1 September 2026\n' > "$root/eula/eula.en.txt"
# The test binary is linked with test-only path macros and never opens host /etc/shadow.
/tmp/http-test >/tmp/divinus-http-test.log 2>&1 & pid=$!
trap 'kill "$pid" 2>/dev/null || true; rm -rf "$root"' EXIT
sleep 1
code=$(curl -sS -o /tmp/divinus-http-body -w '%{http_code}' http://127.0.0.1:18081/eula.en.txt)
test "$code" = 200
test "$(curl -sS -o /dev/null -w '%{http_code}' -X POST -H 'Content-Type: application/x-www-form-urlencoded' --data 'password=abcdefghijkl&confirm=abcdefghijkl&eula=no&eula_lang=en' http://127.0.0.1:18081/setup)" = 400
test "$(curl -sS -o /dev/null -w '%{http_code}' -X POST -H 'Content-Type: application/x-www-form-urlencoded' --data 'password=abcdefghijkl&confirm=abcdefghijkl&eula=accepted&eula_lang=en' http://127.0.0.1:18081/setup)" = 200
test -s "$root/eula-accepted"
test "$(awk -F' ' '{print $3}' "$root/eula-accepted")" = web
echo 'HTTP EULA acceptance tests passed'
