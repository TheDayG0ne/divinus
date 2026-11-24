#!/bin/sh
set -eu
bin=$(mktemp "${TMPDIR:-/tmp}/uvc-jpeg-test.XXXXXX")
trap 'rm -f "$bin"' EXIT HUP INT TERM
${CC:-cc} -std=gnu99 -Wall -Wextra -Isrc/hal tst/uvc-jpeg-api/cache_test.c -o "$bin"
"$bin"
