#!/bin/sh
set -eu
# Run from src, after a normal native build. Objects and binaries stay in container.
gcc -D_GNU_SOURCE -O2 -pthread -c ../claim/http_server_test.c -o /tmp/http-server.o
gcc -D_GNU_SOURCE -O2 -pthread -c ../claim/http_main_test.c -o /tmp/http-main.o
gcc /tmp/http-server.o /tmp/http-main.o $(find . -name '*.o' ! -name server.o ! -name main.o) ../claim/claim.o -pthread -lm -lcrypt -rdynamic -o /tmp/http-test
python3 ../claim/http_test.py
