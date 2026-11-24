#!/bin/sh
set -eu
TEST_DIR=${TEST_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)}
PROJECT_ROOT=${PROJECT_ROOT:-$(CDPATH= cd -- "$TEST_DIR/../.." && pwd)}
bin=$(mktemp "${TMPDIR:-/tmp}/uvc-jpeg-test.XXXXXX")
trap 'rm -f "$bin"' EXIT HUP INT TERM
${CC:-cc} -std=gnu99 -Wall -Wextra -I"$PROJECT_ROOT/src/hal" "$TEST_DIR/cache_test.c" -o "$bin"
"$bin"
