#include "uvc_encoder.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/videodev2.h>

#if defined(UVC_HAVE_FFMPEG)
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

struct uvc_encoder {
    int width, height, fps;
    enum AVPixelFormat input;
    int input_stride;
    AVCodecContext *codec;
    AVCodecContext *decoder;
    AVFrame *frame;
    AVFrame *decoded;
    AVPacket *packet;
    unsigned char *output;
    size_t output_capacity;
    struct SwsContext *scale;
    int64_t pts;
};

static enum AVPixelFormat input_format(uint32_t fmt) {
    return fmt == V4L2_PIX_FMT_YUYV ? AV_PIX_FMT_YUYV422 : AV_PIX_FMT_YUVJ422P;
}

int uvc_encoder_available(void) { return 1; }

struct uvc_encoder *uvc_encoder_create(int width, int height, int fps, int bitrate, int gop, uint32_t fmt, int stride) {
    struct uvc_encoder *e = calloc(1, sizeof(*e));
    const AVCodec *codec, *decoder;
    if (!e || width < 2 || height < 2 || fps < 1) goto fail;
    e->width=width; e->height=height; e->fps=fps; e->input=input_format(fmt); e->input_stride=stride>0?stride:width*2;
    codec=avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) goto fail;
    e->codec=avcodec_alloc_context3(codec);
    if (!e->codec) goto fail;
    e->codec->width=width; e->codec->height=height; e->codec->time_base=(AVRational){1,fps};
    e->codec->framerate=(AVRational){fps,1}; e->codec->pix_fmt=AV_PIX_FMT_YUV420P;
    e->codec->bit_rate=bitrate>0?bitrate:2000000; e->codec->gop_size=gop>0?gop:fps*2;
    e->codec->max_b_frames=0; e->codec->thread_count=1; e->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    av_opt_set(e->codec->priv_data, "preset", "ultrafast", 0);
    av_opt_set(e->codec->priv_data, "tune", "zerolatency", 0);
    av_opt_set(e->codec->priv_data, "repeat-headers", "1", 0);
    av_opt_set(e->codec->priv_data, "annexb", "1", 0);
    if (avcodec_open2(e->codec, codec, NULL)<0) goto fail;
    e->frame=av_frame_alloc(); e->decoded=av_frame_alloc(); e->packet=av_packet_alloc();
    if (!e->frame || !e->decoded || !e->packet) goto fail;
    e->frame->format=e->codec->pix_fmt; e->frame->width=width; e->frame->height=height;
    if (av_frame_get_buffer(e->frame, 32)<0) goto fail;
    if (fmt == V4L2_PIX_FMT_MJPEG) {
        decoder=avcodec_find_decoder(AV_CODEC_ID_MJPEG); if (!decoder) goto fail;
        e->decoder=avcodec_alloc_context3(decoder); if (!e->decoder) goto fail;
        if (avcodec_open2(e->decoder, decoder, NULL)<0) goto fail;
    }
    return e;
fail: uvc_encoder_destroy(e); return NULL;
}

void uvc_encoder_destroy(struct uvc_encoder *e) {
    if (!e) return; if(e->codec) avcodec_free_context(&e->codec); if(e->decoder) avcodec_free_context(&e->decoder);
    if(e->scale) sws_freeContext(e->scale); free(e->output); av_frame_free(&e->frame); av_frame_free(&e->decoded); av_packet_free(&e->packet); free(e);
}

int uvc_encoder_encode(struct uvc_encoder *e, const void *data, size_t len, uint64_t ts, const unsigned char **out, size_t *out_len) {
    AVFrame *src; AVPacket in; int r;
    if(!e||!data||!len||!out||!out_len) return -1;
    if(e->decoder) {
        av_init_packet(&in); in.data=(uint8_t*)data; in.size=(int)len;
        if(avcodec_send_packet(e->decoder,&in)<0 || avcodec_receive_frame(e->decoder,e->decoded)<0) return -1;
        src=e->decoded;
    } else {
        size_t need=(size_t)e->input_stride*e->height; if(len<need) return -1;
        av_frame_unref(e->decoded); e->decoded->format=e->input; e->decoded->width=e->width; e->decoded->height=e->height;
        e->decoded->data[0]=(uint8_t *)data; e->decoded->linesize[0]=e->input_stride; src=e->decoded;
    }
    e->scale=sws_getCachedContext(e->scale,e->width,e->height,src->format,e->width,e->height,AV_PIX_FMT_YUV420P,SWS_BILINEAR,NULL,NULL,NULL);
    if(!e->scale || av_frame_make_writable(e->frame)<0) return -1;
    sws_scale(e->scale,(const uint8_t * const*)src->data,src->linesize,0,e->height,e->frame->data,e->frame->linesize);
    e->frame->pts=e->pts++; r=avcodec_send_frame(e->codec,e->frame); if(r<0)return -1;
    r=avcodec_receive_packet(e->codec,e->packet); if(r==AVERROR(EAGAIN)) return -2; if(r<0)return -1;
    if ((size_t)e->packet->size > e->output_capacity) { unsigned char *p=realloc(e->output,e->packet->size); if(!p)return -1; e->output=p; e->output_capacity=e->packet->size; }
    memcpy(e->output,e->packet->data,e->packet->size); *out=e->output; *out_len=e->packet->size; av_packet_unref(e->packet); (void)ts; return 0;
}
#else
struct uvc_encoder { int unavailable; };
int uvc_encoder_available(void) { return 0; }
struct uvc_encoder *uvc_encoder_create(int w,int h,int fps,int bitrate,int gop,uint32_t fmt,int stride){(void)w;(void)h;(void)fps;(void)bitrate;(void)gop;(void)fmt;(void)stride;return NULL;}
void uvc_encoder_destroy(struct uvc_encoder *e){free(e);}
int uvc_encoder_encode(struct uvc_encoder *e,const void*d,size_t l,uint64_t t,const unsigned char**o,size_t*n){(void)e;(void)d;(void)l;(void)t;(void)o;(void)n;return -1;}
#endif
