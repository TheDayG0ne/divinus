#!/bin/sh
set -eu
cc=${CC:-cc}
out=$(mktemp)
trap 'rm -f "$out"' EXIT
$cc -std=gnu99 -Isrc -Isrc/hal -ffunction-sections -fdata-sections \
    tst/uvc-encoder/jpeg_dispatch_test.c src/jpeg.c -Wl,--gc-sections -pthread -o "$out"
"$out"
