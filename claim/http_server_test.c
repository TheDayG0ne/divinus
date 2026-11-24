/* Test-only build: all mutable paths are confined to the container's /tmp. */
#define CLAIM_SHADOW "/tmp/divinus-http-test/shadow"
#define CLAIM_LOCK "/tmp/divinus-http-test/lock"
#define CLAIM_EULA_DIR "/tmp/divinus-http-test/eula"
#define CLAIM_EULA_RECORD "/tmp/divinus-http-test/eula-accepted"
#define OPENWALL_CONFIG "/tmp/divinus-http-test/openwall.conf"
#define DIVINUS_STATIC_ROOT "/tmp/divinus-http-test/www"
#include "../src/server.c"
