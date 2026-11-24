#define _GNU_SOURCE
#include "shadow_auth.h"
#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__))
#include <crypt.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

static int digit64(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static int decode(const char *s, unsigned char *out, size_t cap) {
    size_t n = strlen(s), j = 0;
    if (!n || n > 512 || n % 4) return -1;
    for (size_t i = 0; i < n; i += 4) {
        int a = digit64(s[i]), b = digit64(s[i+1]);
        int c = s[i+2] == '=' ? -2 : digit64(s[i+2]);
        int d = s[i+3] == '=' ? -2 : digit64(s[i+3]);
        if (a < 0 || b < 0 || c == -1 || d == -1 ||
            (c == -2 && (d != -2 || (b & 15))) ||
            (d == -2 && c >= 0 && (c & 3)) ||
            ((c == -2 || d == -2) && i + 4 != n)) return -1;
        if (j + 3 >= cap) return -1;
        out[j++] = (a << 2) | (b >> 4);
        if (c >= 0) out[j++] = (b << 4) | (c >> 2);
        if (d >= 0) out[j++] = (c << 6) | d;
    }
    if (memchr(out, 0, j)) return -1;
    out[j] = 0;
    return (int)j;
}

int shadow_auth_basic(const char *path, const char *authorization) {
    unsigned char credentials[388] = {0};
    char *line = NULL, *hash = NULL;
    size_t cap = 0;
    ssize_t n;
    int fd, roots = 0, ok = 0;
    FILE *file;
    struct stat st;
    struct crypt_data *data = NULL;
    if (!path || !authorization || strncasecmp(authorization, "Basic ", 6) ||
        decode(authorization + 6, credentials, sizeof(credentials)) < 0 ||
        strncmp((char *)credentials, "root:", 5) || !credentials[5]) goto done;
    fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) goto done;
    if (fstat(fd, &st) || !S_ISREG(st.st_mode)) { close(fd); goto done; }
    file = fdopen(fd, "r");
    if (!file) { close(fd); goto done; }
    while ((n = getline(&line, &cap, file)) >= 0) {
        if (memchr(line, 0, n)) { roots = -1; break; }
        if (!strncmp(line, "root:", 5)) {
            char *end = strchr(line + 5, ':');
            int colons = 0;
            for (ssize_t i = 0; i < n; i++) if (line[i] == ':') colons++;
            if (++roots != 1 || !end || n < 1 || line[n-1] != '\n' ||
                memchr(line, '\r', n) || colons != 8 || end == line + 5 ||
                line[5] == '!' || line[5] == '*') { roots = -1; break; }
            hash = strndup(line + 5, end - line - 5);
            if (!hash) { roots = -1; break; }
        }
    }
    if (ferror(file)) roots = -1;
    if (fclose(file)) roots = -1;
    if (roots != 1 || !hash) goto done;
    data = calloc(1, sizeof(*data));
    if (data) {
        char *result = crypt_r((char *)credentials + 5, hash, data);
        if (result && result[0] != '*' && !strcmp(result, hash)) ok = 1;
    }
done:
    if (data) { explicit_bzero(data, sizeof(*data)); free(data); }
    if (hash) { explicit_bzero(hash, strlen(hash)); free(hash); }
    if (line) { explicit_bzero(line, cap); free(line); }
    explicit_bzero(credentials, sizeof(credentials));
    return ok;
}
#else
int shadow_auth_basic(const char *path, const char *authorization) {
    (void)path; (void)authorization; return 0;
}
#endif
