#!/bin/sh
set -eu
cc=${CC:-cc}
flags="-std=gnu99 -DUVC_HAVE_FFMPEG -Isrc -Isrc/hal $(pkg-config --cflags libavcodec libavutil libswscale)"
$cc $flags "$TEST_DIR/uvc_encoder_test.c" src/hal/uvc_encoder.c $(pkg-config --libs libavcodec libavutil libswscale) -o uvc_encoder_test
./uvc_encoder_test
ffprobe -v error -f h264 -show_streams uvc-test.h264 >/dev/null
