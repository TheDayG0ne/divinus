#include "jpeg.h"
#include <assert.h>

struct AppConfig app_config;
hal_platform plat = HAL_PLATFORM_UVC;
static int bound, snapshots, fail_bind;
int uvc_channel_bind(char index) { (void)index; bound++; return fail_bind; }
int uvc_video_snapshot_grab(char index, hal_jpegdata *jpeg) {
    (void)index;
    snapshots++;
    jpeg->data = malloc(1);
    assert(jpeg->data);
    jpeg->data[0] = 42;
    jpeg->length = jpeg->jpegSize = 1;
    return 0;
}
int take_next_free_channel(bool mainLoop) { (void)mainLoop; return 0; }
int create_channel(char index, short width, short height, char framerate, char jpeg) {
    (void)index; (void)width; (void)height; (void)framerate; (void)jpeg; return 0;
}
char *errstr(int error) { (void)error; return "error"; }

int main(void) {
    hal_jpegdata jpeg = {0};
    fail_bind = 1;
    assert(jpeg_init() != 0);
    assert(jpeg_get(0, 0, 0, 0, &jpeg) != 0);
    fail_bind = 0;
    assert(jpeg_init() == 0 && bound == 2);
    assert(jpeg_get(0, 0, 0, 0, &jpeg) == 0);
    assert(snapshots == 1 && jpeg.jpegSize == 1 && jpeg.data[0] == 42);
    free(jpeg.data);
    jpeg_deinit();
    return 0;
}
