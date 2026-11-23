#include "uvc_hal.h"
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <errno.h>

#define UVC_BUFS 4
struct uvc_buf { void *data; size_t len; };
static int fd = -1, width, height, streaming;
static unsigned int buf_count;
static uint32_t pixel_format;
static struct uvc_buf bufs[UVC_BUFS];
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static unsigned char *last; static size_t last_len;
hal_chnstate uvc_state[UVC_MAX_CHANNELS];
int (*uvc_vid_cb)(char, hal_vidstream*);

static int ctl(unsigned long req, void *arg) { int r; do r=ioctl(fd,req,arg); while(r<0 && errno==EINTR); return r; }
static void close_device(void) {
    if (fd < 0) return;
    if (streaming) { enum v4l2_buf_type t=V4L2_BUF_TYPE_VIDEO_CAPTURE; ctl(VIDIOC_STREAMOFF,&t); streaming=0; }
    for (int i=0;i<UVC_BUFS;i++) if (bufs[i].data) munmap(bufs[i].data,bufs[i].len);
    close(fd); fd=-1; buf_count=0; free(last); last=NULL; last_len=0;
}
int uvc_hal_init(void) { memset(uvc_state,0,sizeof(uvc_state)); return 0; }
void uvc_hal_deinit(void) { close_device(); }
int uvc_system_init(char *config) { (void)config; return 0; }
void uvc_system_deinit(void) { }
int uvc_pipeline_create() {
    short w = 1280, h = 720;
    const char *path=getenv("UVC_DEVICE"); if(!path||!*path) path="/dev/video0";
    fd=open(path,O_RDWR|O_NONBLOCK); if(fd<0) return -1;
    struct v4l2_capability cap; if(ctl(VIDIOC_QUERYCAP,&cap)<0 || !(cap.capabilities&V4L2_CAP_VIDEO_CAPTURE) || !(cap.capabilities&V4L2_CAP_STREAMING)) { close_device(); return -1; }
    struct v4l2_format f; memset(&f,0,sizeof(f)); f.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    f.fmt.pix.width=w>0?w:1280; f.fmt.pix.height=h>0?h:720; f.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG; f.fmt.pix.field=V4L2_FIELD_ANY;
    if(ctl(VIDIOC_S_FMT,&f)<0) { f.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV; if(ctl(VIDIOC_S_FMT,&f)<0) { close_device(); return -1; } }
    if(f.fmt.pix.pixelformat!=V4L2_PIX_FMT_MJPEG && f.fmt.pix.pixelformat!=V4L2_PIX_FMT_YUYV) { close_device(); return -1; }
    if(f.fmt.pix.width < 2 || f.fmt.pix.height < 2 || f.fmt.pix.sizeimage == 0) { close_device(); return -1; }
    width=f.fmt.pix.width; height=f.fmt.pix.height; pixel_format=f.fmt.pix.pixelformat;
    struct v4l2_requestbuffers rb; memset(&rb,0,sizeof(rb)); rb.count=UVC_BUFS; rb.type=f.type; rb.memory=V4L2_MEMORY_MMAP;
    if(ctl(VIDIOC_REQBUFS,&rb)<0 || rb.count<2 || rb.count>UVC_BUFS) { close_device(); return -1; }
    buf_count=rb.count;
    for(unsigned i=0;i<buf_count;i++) { struct v4l2_buffer b; memset(&b,0,sizeof(b)); b.type=f.type;b.memory=V4L2_MEMORY_MMAP;b.index=i; if(ctl(VIDIOC_QUERYBUF,&b)<0) {close_device();return -1;} bufs[i].len=b.length; bufs[i].data=mmap(NULL,b.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,b.m.offset); if(bufs[i].data==MAP_FAILED){bufs[i].data=NULL;close_device();return -1;} if(ctl(VIDIOC_QBUF,&b)<0){close_device();return -1;} }
    return 0;
}
void uvc_pipeline_destroy(void) { pthread_mutex_lock(&mtx); close_device(); pthread_mutex_unlock(&mtx); }
void uvc_pipeline_streamon(void) { if(fd>=0&&!streaming){enum v4l2_buf_type t=V4L2_BUF_TYPE_VIDEO_CAPTURE;if(!ctl(VIDIOC_STREAMON,&t))streaming=1;} }
int uvc_channel_create(char i, short w, short h, char fps, char jpeg){(void)w;(void)h;(void)fps;(void)jpeg; return i>=0&&i<UVC_MAX_CHANNELS?0:-1;}
int uvc_channel_bind(char i){(void)i; uvc_pipeline_streamon(); return streaming?0:-1;}
int uvc_channel_unbind(char i){(void)i; return 0;}
int uvc_video_create(char i, hal_vidconfig *c){if(i<0||i>=UVC_MAX_CHANNELS||!c)return -1;if(c->codec!=HAL_VIDCODEC_MJPG&&c->codec!=HAL_VIDCODEC_JPG)return -1;uvc_state[i].payload=c->codec;return 0;}
int uvc_video_destroy(char i){if(i<0||i>=UVC_MAX_CHANNELS)return -1;uvc_state[i].enable=0;return 0;}
int uvc_video_destroy_all(void){for(int i=0;i<UVC_MAX_CHANNELS;i++)uvc_video_destroy(i);return 0;}
int uvc_video_snapshot_grab(char i, hal_jpegdata *j){(void)i;if(!j)return -1;memset(j,0,sizeof(*j));pthread_mutex_lock(&mtx);if(last_len){j->data=malloc(last_len);if(j->data){memcpy(j->data,last,last_len);j->length=j->jpegSize=last_len;}}pthread_mutex_unlock(&mtx);return j->data?0:-1;}
void uvc_video_request_idr(char i){(void)i;}
int uvc_channel_grayscale(char e){(void)e;return 0;}
int uvc_audio_init(int r){(void)r;return -1;} void uvc_audio_deinit(void){} void *uvc_audio_thread(void){return NULL;}
int uvc_region_create(char h,hal_rect r,short o){(void)h;(void)r;(void)o;return 0;} int uvc_region_setbitmap(char h,hal_bitmap*b){(void)h;(void)b;return -1;} void uvc_region_destroy(char h){(void)h;}
void *uvc_video_thread(void){
    while(keepRunning){ struct pollfd p={fd,POLLIN,0}; if(fd<0){usleep(10000);continue;} if(poll(&p,1,1000)<=0)continue; if(!(p.revents&POLLIN))continue;
        struct v4l2_buffer b; memset(&b,0,sizeof(b));b.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;b.memory=V4L2_MEMORY_MMAP; if(ctl(VIDIOC_DQBUF,&b)<0)continue;
        pthread_mutex_lock(&mtx);
        if (b.index >= buf_count || b.bytesused > bufs[b.index].len) { pthread_mutex_unlock(&mtx); continue; }
        unsigned char *frame = bufs[b.index].data;
        if (pixel_format == V4L2_PIX_FMT_MJPEG) {
            unsigned char *n=realloc(last,b.bytesused); if(n){last=n;memcpy(last,frame,b.bytesused);last_len=b.bytesused;}
        }
        for(int i=0;i<UVC_MAX_CHANNELS;i++) if(uvc_state[i].enable&&uvc_state[i].mainLoop&&uvc_vid_cb){hal_vidpack pack;hal_vidstream s;memset(&pack,0,sizeof(pack));pack.data=frame;pack.length=b.bytesused;pack.naluCnt=1;pack.nalu[0].length=b.bytesused;pack.timestamp=(uint64_t)b.timestamp.tv_sec*1000000+b.timestamp.tv_usec;s.pack=&pack;s.count=1;s.seq=0;uvc_vid_cb(i,&s);}
        pthread_mutex_unlock(&mtx);
        ctl(VIDIOC_QBUF,&b);
    } return NULL;
}
