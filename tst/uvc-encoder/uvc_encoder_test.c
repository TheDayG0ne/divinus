#include "../../src/hal/uvc_encoder.h"
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const int w=320,h=240,stride=w*2;
    unsigned char *yuyv=malloc((size_t)stride*h);
    if (!yuyv) return 1;
    struct uvc_encoder *e=uvc_encoder_create(w,h,30,500000,30,V4L2_PIX_FMT_YUYV,stride);
    if (!e) { free(yuyv); return 2; }
    int saw_sps=0,saw_pps=0,saw_idr=0,frames=0;
    FILE *stream=fopen("uvc-test.h264","wb"); if(!stream) return 5;
    for (int f=0; f<60; f++) {
        for (int y=0;y<h;y++) for (int x=0;x<w;x+=2) {
            unsigned char *p=yuyv+(size_t)y*stride+x*2;
            p[0]=(unsigned char)(32+(y*180/h)); p[1]=(unsigned char)(64+(x*160/w));
            p[2]=p[0]; p[3]=(unsigned char)(128+(y*80/h));
        }
        const unsigned char *out; size_t len;
        int r=uvc_encoder_encode(e,yuyv,(size_t)stride*h,(uint64_t)f*33333,&out,&len);
        if (r==0) {
            frames++; fwrite(out,1,len,stream);
            for (size_t i=0;i+4<len;i++) if(!out[i]&&!out[i+1]&&out[i+2]==0&&out[i+3]==1) {
                int type=out[i+4]&31; saw_sps|=type==7; saw_pps|=type==8; saw_idr|=type==5;
            }
        } else if (r != -2) { uvc_encoder_destroy(e); free(yuyv); return 3; }
    }
    fclose(stream); uvc_encoder_destroy(e); free(yuyv);
    if (!frames || !saw_sps || !saw_pps || !saw_idr) { fprintf(stderr,"frames=%d sps=%d pps=%d idr=%d\n",frames,saw_sps,saw_pps,saw_idr); return 4; }
    return 0;
}
