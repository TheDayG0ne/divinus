#!/bin/sh
set -eu
cd "$(dirname "$0")"
bin=$(mktemp /tmp/divinus-shadow-auth-test.XXXXXX)
trap 'rm -f "$bin"' EXIT
${CC:-cc} -Wall -Wextra -O2 -pthread test.c ../../src/shadow_auth.c -lcrypt -o "$bin"
"$bin"
