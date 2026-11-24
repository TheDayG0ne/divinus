#pragma once

/* Returns 1 only for valid root Basic credentials against this shadow file. */
int shadow_auth_basic(const char *path, const char *authorization);
