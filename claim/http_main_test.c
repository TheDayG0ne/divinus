#include "../src/server.h"
#include "../src/rtsp/rtsp_server.h"
volatile char graceful = 0, keepRunning = 1;
rtsp_handle rtspHandle;
int main(void) {
    signal(SIGPIPE, SIG_IGN);
    app_config.web_port = 18081;
    server_start();
    for (;;) pause();
}
