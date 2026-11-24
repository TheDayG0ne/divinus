#include "../src/server.h"
#include "../src/rtsp/rtsp_server.h"
volatile char graceful = 0, keepRunning = 1;
rtsp_handle rtspHandle;
int main(void) {
    signal(SIGPIPE, SIG_IGN);
    app_config.web_port = 18081;
    app_config.web_enable_static = true;
    app_config.web_enable_auth = true;
    app_config.web_auth_skiplocal = false;
    strcpy(app_config.web_auth_user, "admin");
    strcpy(app_config.web_auth_pass, "secret");
    server_start();
    for (;;) pause();
}
