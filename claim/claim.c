#define _GNU_SOURCE
#include "claim.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int claim_set_hash(const char *shadow_path, const char *lock_path, const char *hash)
{
    int lock = -1, fd = -1, result = -1, roots = 0, changed = 0;
    FILE *in = NULL, *out = NULL;
    char *line = NULL, *tmp = NULL;
    size_t cap = 0;
    ssize_t n;
    struct stat st;

    /* Reject lock markers and delimiters: this API accepts only crypt hashes. */
    if (!shadow_path || !lock_path || !hash || hash[0] != '$' ||
        !strchr(hash + 1, '$') || strpbrk(hash, ":\r\n")) return -1;
    lock = open(lock_path, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (lock < 0 || fstat(lock, &st) || !S_ISREG(st.st_mode) || flock(lock, LOCK_EX)) goto done;
    fd = open(shadow_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0 || fstat(fd, &st) || !S_ISREG(st.st_mode)) goto done;
    in = fdopen(fd, "r");
    if (!in) goto done;
    fd = -1;
    if (asprintf(&tmp, "%s.claim.XXXXXX", shadow_path) < 0) { tmp = NULL; goto done; }
    fd = mkstemp(tmp);
    if (fd < 0 || fchown(fd, st.st_uid, st.st_gid) || fchmod(fd, st.st_mode & 07777)) goto done;
    out = fdopen(fd, "w");
    if (!out) goto done;
    fd = -1;
    while ((n = getline(&line, &cap, in)) >= 0) {
        if (memchr(line, '\0', (size_t)n)) goto done;
        if (!strncmp(line, "root:", 5)) {
            char *end = strchr(line + 5, ':');
            if (++roots != 1 || !end || !strchr(end, '\n')) goto done;
            if (end != line + 5) {
                if (fwrite(line, 1, n, out) != (size_t)n) goto done;
            } else {
                if (fprintf(out, "root:%s%s", hash, end) < 0) goto done;
                changed = 1;
            }
        } else if (fwrite(line, 1, n, out) != (size_t)n) goto done;
    }
    if (ferror(in) || roots != 1) goto done;
    if (!changed) { result = 1; goto done; }
    if (fflush(out) || fsync(fileno(out))) goto done;
    if (fclose(out)) { out = NULL; goto done; }
    out = NULL;
    if (rename(tmp, shadow_path)) goto done;
    /* A failure to sync the directory is reported even though rename occurred. */
    {
        char *slash = strrchr(tmp, '/');
        int dir;
        if (slash) { if (slash == tmp) slash[1] = '\0'; else *slash = '\0'; }
        dir = open(slash ? tmp : ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (dir < 0) { free(tmp); tmp = NULL; goto done; }
        result = fsync(dir) ? -1 : 0;
        close(dir);
        free(tmp); tmp = NULL;
    }
done:
    free(line);
    if (out) fclose(out);
    if (in) fclose(in);
    if (fd >= 0) close(fd);
    if (tmp) { unlink(tmp); free(tmp); }
    if (lock >= 0) close(lock);
    return result;
}
