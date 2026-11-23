#pragma once
#include <stddef.h>
#include <stdint.h>

struct uvc_encoder;
struct uvc_encoder *uvc_encoder_create(int width, int height, int fps, int bitrate, int gop, uint32_t input_format);
void uvc_encoder_destroy(struct uvc_encoder *encoder);
int uvc_encoder_encode(struct uvc_encoder *encoder, const void *data, size_t len, uint64_t timestamp, const unsigned char **out, size_t *out_len);
int uvc_encoder_available(void);
