#define _GNU_SOURCE
#include "../../src/shadow_auth.h"
#include <assert.h>
#include <crypt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char path[] = "/tmp/divinus-shadow-auth-XXXXXX";
static void shadow(const char *hash) {
    FILE *f = fopen(path, "w"); assert(f);
    assert(fprintf(f, "root:%s:19000:0:99999:7:::\n", hash) > 0);
    assert(!fclose(f));
}
static void *worker(void *unused) {
    (void)unused;
    for (int i = 0; i < 20; i++) {
        assert(shadow_auth_basic(path, "Basic cm9vdDpzZWNyZXQ="));
        assert(!shadow_auth_basic(path, "Basic cm9vdDpiYWQ="));
    }
    return NULL;
}
int main(void) {
    int fd = mkstemp(path); assert(fd >= 0); close(fd);
    char *hash = crypt("secret", "$6$test$"); assert(hash && hash[0] != '*');
    shadow(hash);
    assert(shadow_auth_basic(path, "basic cm9vdDpzZWNyZXQ="));
    assert(!shadow_auth_basic(path, NULL));
    assert(!shadow_auth_basic(path, "Basic YWRtaW46c2VjcmV0"));
    assert(!shadow_auth_basic(path, "Basic cm9vdDpzZWNyZXQ=garbage"));
    assert(!shadow_auth_basic(path, "Basic cm9vdDoA"));
    pthread_t threads[4];
    for (int i = 0; i < 4; i++) assert(!pthread_create(&threads[i], NULL, worker, NULL));
    for (int i = 0; i < 4; i++) assert(!pthread_join(threads[i], NULL));
    shadow("!"); assert(!shadow_auth_basic(path, "Basic cm9vdDpzZWNyZXQ="));
    shadow(""); assert(!shadow_auth_basic(path, "Basic cm9vdDpzZWNyZXQ="));
    unlink(path); assert(!shadow_auth_basic(path, "Basic cm9vdDpzZWNyZXQ="));
    return 0;
}
