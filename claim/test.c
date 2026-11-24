#define _GNU_SOURCE
#include "claim.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void put(const char *path, const char *text) {
    FILE *f = fopen(path, "w"); assert(f); assert(fputs(text, f) >= 0); assert(!fclose(f));
}
int main(void) {
    char dir[] = "/tmp/divinus-claim-XXXXXX", shadow[256], lock[256], buf[1024];
    FILE *f; int status, a, b; pid_t p;
    assert(mkdtemp(dir));
    snprintf(shadow, sizeof shadow, "%s/shadow", dir);
    snprintf(lock, sizeof lock, "%s/lock", dir);
    put(shadow, "root::1:2:3:4:5:6:7\nuser:!:1:2:3:4:5:6:7\n");
    assert(claim_state(shadow) == 0);
    assert(claim_state("/nonexistent/divinus-shadow") == -1);
    assert(claim_set_hash(shadow, lock, "bad:hash") == -1);
    p = fork(); assert(p >= 0);
    if (!p) _exit(claim_set_hash(shadow, lock, "$6$salt$first"));
    b = claim_set_hash(shadow, lock, "$6$salt$second");
    assert(waitpid(p, &status, 0) == p && WIFEXITED(status)); a = WEXITSTATUS(status);
    assert((a == 0 && b == 1) || (a == 1 && b == 0));
    f = fopen(shadow, "r"); assert(f); assert(fgets(buf, sizeof buf, f));
    assert(!strcmp(buf, "root:$6$salt$first:1:2:3:4:5:6:7\n") ||
           !strcmp(buf, "root:$6$salt$second:1:2:3:4:5:6:7\n"));
    assert(fgets(buf, sizeof buf, f) && !strcmp(buf, "user:!:1:2:3:4:5:6:7\n")); fclose(f);
    put(shadow, "root:!:1:2:3:4:5:6:7\n");
    assert(claim_set_hash(shadow, lock, "$6$salt$hash") == 1);
    assert(claim_state(shadow) == 1);
    put(shadow, "user::1:2:3:4:5:6:7\n");
    assert(claim_state(shadow) == -1);
    assert(claim_set_hash(shadow, lock, "$6$salt$hash") == -1);
    put(shadow, "root::1:2:3:4:5:6:7\nroot::1:2:3:4:5:6:7\n");
    assert(claim_set_hash(shadow, lock, "$6$salt$hash") == -1);
    assert(claim_state(shadow) == -1);
    put(shadow, "root::1:2:3:4:5:6:7\nroot:!:1:2:3:4:5:6:7\n");
    assert(claim_state(shadow) == -1);
    put(shadow, "root::\n");
    assert(claim_state(shadow) == -1);
    assert(claim_set_hash(shadow, lock, "$6$salt$hash") == -1);
    put(shadow, "root:$6$salt$hash:1:2:3:4:5:6\n");
    assert(claim_state(shadow) == -1);
    assert(claim_set_hash(shadow, lock, "$6$salt$hash") == -1);
    put(shadow, "root::1:2:3:4:5:6:7:extra\n");
    assert(claim_state(shadow) == -1);
    assert(claim_set_hash(shadow, lock, "$6$salt$hash") == -1);
    unlink(shadow); unlink(lock); rmdir(dir);
    puts("claim tests passed"); return 0;
}
