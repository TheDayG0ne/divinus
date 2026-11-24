#include "uvc_hal.h"
#include "uvc_encoder.h"
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <errno.h>

#define UVC_BUFS 4
struct uvc_buf { void *data; size_t len; };
static int fd = -1, width, height, streaming;
/* Device state is protected by mtx; only the video thread opens it. */
static int pipeline_active, capture_requested;
static unsigned int buf_count;
static uint32_t pixel_format;
static int input_stride, fps=30;
static struct uvc_buf bufs[UVC_BUFS];
static struct uvc_encoder *encoder;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static unsigned char *last; static size_t last_len; static uint64_t last_monotonic_us;
hal_chnstate uvc_state[UVC_MAX_CHANNELS];
int (*uvc_vid_cb)(char, hal_vidstream*);

static int ctl(unsigned long req, void *arg) { int r; do r=ioctl(fd,req,arg); while(r<0 && errno==EINTR); return r; }
static void close_device(void) {
    if (fd < 0) return;
    if (streaming) { enum v4l2_buf_type t=V4L2_BUF_TYPE_VIDEO_CAPTURE; ctl(VIDIOC_STREAMOFF,&t); streaming=0; }
    for (int i=0;i<UVC_BUFS;i++) if (bufs[i].data) { munmap(bufs[i].data,bufs[i].len); bufs[i].data=NULL; bufs[i].len=0; }
    close(fd); fd=-1; buf_count=0; uvc_encoder_destroy(encoder); encoder=NULL;
    free(last); last=NULL; last_len=0; last_monotonic_us=0;
}

static uint64_t monotonic_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

uvc_jpeg_result uvc_jpeg_get(unsigned int max_age_ms, size_t max_length,
    uvc_jpeg_frame *out) {
    uint64_t now, age;
    if (!out || !max_age_ms || !max_length) return UVC_JPEG_INVALID;
    memset(out, 0, sizeof(*out));
    pthread_mutex_lock(&mtx);
    if (pixel_format == V4L2_PIX_FMT_YUYV) {
        pthread_mutex_unlock(&mtx);
        return UVC_JPEG_UNSUPPORTED;
    }
    if (!last || !last_len || !last_monotonic_us) {
        pthread_mutex_unlock(&mtx);
        return UVC_JPEG_NO_FRAME;
    }
    now = monotonic_us();
    age = now >= last_monotonic_us ? now - last_monotonic_us : UINT64_MAX;
    if (age > (uint64_t)max_age_ms * 1000ULL) {
        pthread_mutex_unlock(&mtx);
        return UVC_JPEG_STALE;
    }
    if (last_len > max_length) {
        pthread_mutex_unlock(&mtx);
        return UVC_JPEG_TOO_LARGE;
    }
    out->data = malloc(last_len);
    if (!out->data) {
        pthread_mutex_unlock(&mtx);
        return UVC_JPEG_INVALID;
    }
    memcpy(out->data, last, last_len);
    out->length = last_len;
    out->captured_monotonic_us = last_monotonic_us;
    pthread_mutex_unlock(&mtx);
    return UVC_JPEG_OK;
}

void uvc_jpeg_frame_release(uvc_jpeg_frame *frame) {
    if (!frame) return;
    free(frame->data);
    memset(frame, 0, sizeof(*frame));
}
int uvc_hal_init(void) { memset(uvc_state,0,sizeof(uvc_state)); return 0; }
void uvc_hal_deinit(void) { uvc_pipeline_destroy(); }
int uvc_system_init(char *config) { (void)config; return 0; }
void uvc_system_deinit(void) { }
/* Called with mtx held by the capture thread. */
static int open_device(void) {
    short w = 1280, h = 720;
    const char *path=getenv("UVC_DEVICE"); if(!path||!*path) path="/dev/video0";
    fd=open(path,O_RDWR|O_NONBLOCK); if(fd<0) return -1;
    struct v4l2_capability cap; memset(&cap,0,sizeof(cap));
    if(ctl(VIDIOC_QUERYCAP,&cap)<0) { close_device(); return -1; }
    uint32_t caps=(cap.capabilities&V4L2_CAP_DEVICE_CAPS)?cap.device_caps:cap.capabilities;
    if (!(caps&V4L2_CAP_VIDEO_CAPTURE) || !(caps&V4L2_CAP_STREAMING)) { close_device(); return -1; }
    struct v4l2_format f; memset(&f,0,sizeof(f)); f.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    f.fmt.pix.width=w>0?w:1280; f.fmt.pix.height=h>0?h:720; f.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG; f.fmt.pix.field=V4L2_FIELD_ANY;
    if(ctl(VIDIOC_S_FMT,&f)<0) { f.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV; if(ctl(VIDIOC_S_FMT,&f)<0) { close_device(); return -1; } }
    if(f.fmt.pix.pixelformat!=V4L2_PIX_FMT_MJPEG && f.fmt.pix.pixelformat!=V4L2_PIX_FMT_YUYV) { close_device(); return -1; }
    if(f.fmt.pix.width < 2 || f.fmt.pix.height < 2 || f.fmt.pix.sizeimage == 0) { close_device(); return -1; }
    width=f.fmt.pix.width; height=f.fmt.pix.height; pixel_format=f.fmt.pix.pixelformat; input_stride=f.fmt.pix.bytesperline; if(input_stride<=0) input_stride=width*2;
    struct v4l2_streamparm parm; memset(&parm,0,sizeof(parm)); parm.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ctl(VIDIOC_G_PARM,&parm)==0 && parm.parm.capture.timeperframe.numerator && parm.parm.capture.timeperframe.denominator)
        fps=(int)(parm.parm.capture.timeperframe.denominator/parm.parm.capture.timeperframe.numerator);
    if (fps<1) fps=30;
    struct v4l2_requestbuffers rb; memset(&rb,0,sizeof(rb)); rb.count=UVC_BUFS; rb.type=f.type; rb.memory=V4L2_MEMORY_MMAP;
    if(ctl(VIDIOC_REQBUFS,&rb)<0 || rb.count<2 || rb.count>UVC_BUFS) { close_device(); return -1; }
    buf_count=rb.count;
    for(unsigned i=0;i<buf_count;i++) { struct v4l2_buffer b; memset(&b,0,sizeof(b)); b.type=f.type;b.memory=V4L2_MEMORY_MMAP;b.index=i; if(ctl(VIDIOC_QUERYBUF,&b)<0) {close_device();return -1;} bufs[i].len=b.length; bufs[i].data=mmap(NULL,b.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,b.m.offset); if(bufs[i].data==MAP_FAILED){bufs[i].data=NULL;close_device();return -1;} if(ctl(VIDIOC_QBUF,&b)<0){close_device();return -1;} }
    encoder=uvc_encoder_create(width,height,fps,2000000,fps*2,pixel_format,input_stride);
    if (!encoder) { close_device(); return -1; }
    return 0;
}
int uvc_pipeline_create(void) { pthread_mutex_lock(&mtx); pipeline_active=1; capture_requested=0; pthread_mutex_unlock(&mtx); return 0; }
void uvc_pipeline_destroy(void) { pthread_mutex_lock(&mtx); pipeline_active=0; capture_requested=0; close_device(); pthread_mutex_unlock(&mtx); }
int uvc_channel_create(char i, short w, short h, char fps, char jpeg){(void)w;(void)h;(void)fps;(void)jpeg; return i>=0&&i<UVC_MAX_CHANNELS?0:-1;}
int uvc_channel_bind(char i){if(i<0||i>=UVC_MAX_CHANNELS)return -1; pthread_mutex_lock(&mtx); int active=pipeline_active; if(active) capture_requested=1; pthread_mutex_unlock(&mtx); return active?0:-1;}
int uvc_channel_unbind(char i){(void)i; return 0;}
int uvc_video_create(char i, hal_vidconfig *c){if(i<0||i>=UVC_MAX_CHANNELS||!c)return -1;if(c->codec!=HAL_VIDCODEC_H264)return -1;uvc_state[i].payload=HAL_VIDCODEC_H264;return 0;}
int uvc_video_destroy(char i){if(i<0||i>=UVC_MAX_CHANNELS)return -1;uvc_state[i].enable=0;return 0;}
int uvc_video_destroy_all(void){for(int i=0;i<UVC_MAX_CHANNELS;i++)uvc_video_destroy(i);return 0;}
int uvc_video_snapshot_grab(char i, hal_jpegdata *j){(void)i;if(!j)return -1;memset(j,0,sizeof(*j));pthread_mutex_lock(&mtx);if(last_len){j->data=malloc(last_len);if(j->data){memcpy(j->data,last,last_len);j->length=j->jpegSize=last_len;}}pthread_mutex_unlock(&mtx);return j->data?0:-1;}
void uvc_video_request_idr(char i){(void)i;}
int uvc_channel_grayscale(char e){(void)e;return 0;}
int uvc_audio_init(int r){(void)r;return -1;} void uvc_audio_deinit(void){} void *uvc_audio_thread(void){return NULL;}
int uvc_region_create(char h,hal_rect r,short o){(void)h;(void)r;(void)o;return 0;} int uvc_region_setbitmap(char h,hal_bitmap*b){(void)h;(void)b;return -1;} void uvc_region_destroy(char h){(void)h;}
void *uvc_video_thread(void){
    while(keepRunning){
        pthread_mutex_lock(&mtx);
        if (!pipeline_active || !capture_requested) { pthread_mutex_unlock(&mtx); usleep(250000); continue; }
        if (fd<0) {
            if (open_device()<0) { pthread_mutex_unlock(&mtx); usleep(1000000); continue; }
            enum v4l2_buf_type t=V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if (ctl(VIDIOC_STREAMON,&t)<0) { close_device(); pthread_mutex_unlock(&mtx); usleep(1000000); continue; }
            streaming=1;
        }
        struct pollfd p={fd,POLLIN,0};
        int ready=poll(&p,1,250);
        if (ready<0 && errno==EINTR) { pthread_mutex_unlock(&mtx); continue; }
        if (ready<0 || (ready>0 && (p.revents&(POLLERR|POLLHUP|POLLNVAL)))) { close_device(); pthread_mutex_unlock(&mtx); usleep(1000000); continue; }
        if (ready==0 || !(p.revents&POLLIN)) { pthread_mutex_unlock(&mtx); continue; }
        struct v4l2_buffer b; memset(&b,0,sizeof(b));b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;b.memory=V4L2_MEMORY_MMAP;
        if(ctl(VIDIOC_DQBUF,&b)<0) { if(errno!=EAGAIN) close_device(); pthread_mutex_unlock(&mtx); continue; }
        if (b.index >= buf_count || b.bytesused > bufs[b.index].len) { close_device(); pthread_mutex_unlock(&mtx); continue; }
        unsigned char *frame = bufs[b.index].data;
        unsigned char *encoded=NULL; size_t encoded_len=0;
        if (pixel_format == V4L2_PIX_FMT_MJPEG) {
            unsigned char *n=realloc(last,b.bytesused); if(n){last=n;memcpy(last,frame,b.bytesused);last_len=b.bytesused;last_monotonic_us=monotonic_us();}
        }
        if (uvc_encoder_encode(encoder,frame,b.bytesused,(uint64_t)b.timestamp.tv_sec*1000000+b.timestamp.tv_usec,(const unsigned char **)&encoded,&encoded_len)<0) { if(ctl(VIDIOC_QBUF,&b)<0) close_device(); pthread_mutex_unlock(&mtx); continue; }
        unsigned char *callback_frame=malloc(encoded_len);
        if (!callback_frame) { if(ctl(VIDIOC_QBUF,&b)<0) close_device(); pthread_mutex_unlock(&mtx); continue; }
        memcpy(callback_frame,encoded,encoded_len);
        if(ctl(VIDIOC_QBUF,&b)<0) close_device();
        pthread_mutex_unlock(&mtx);
        for (size_t pos=0; pos+3<encoded_len;) {
            size_t start=pos, sc=0;
            while (start+3<encoded_len) {
                if (!callback_frame[start] && !callback_frame[start+1]) {
                    if (callback_frame[start+2]==1) { sc=3; break; }
                    if (start+4<encoded_len && !callback_frame[start+2] && callback_frame[start+3]==1) { sc=4; break; }
                }
                start++;
            }
            if (!sc || start+sc>=encoded_len) break;
            size_t end=start+sc+1;
            while (end+3<encoded_len && !(callback_frame[end]==0 && callback_frame[end+1]==0 &&
                (callback_frame[end+2]==1 || (end+3<encoded_len && callback_frame[end+2]==0 && callback_frame[end+3]==1)))) end++;
            if (end+3>=encoded_len) end=encoded_len;
            for(int i=0;i<UVC_MAX_CHANNELS;i++) if(uvc_state[i].enable&&uvc_state[i].mainLoop&&uvc_vid_cb){hal_vidpack pack;hal_vidstream s;memset(&pack,0,sizeof(pack));pack.data=callback_frame+start;pack.length=end-start;pack.naluCnt=1;pack.nalu[0].length=end-start;pack.nalu[0].type=callback_frame[start+sc]&31;pack.timestamp=(uint64_t)b.timestamp.tv_sec*1000000+b.timestamp.tv_usec;s.pack=&pack;s.count=1;s.seq=0;uvc_vid_cb(i,&s);}
            pos=end;
        }
        free(callback_frame);
    } return NULL;
}
