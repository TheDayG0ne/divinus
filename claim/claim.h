#ifndef DIVINUS_CLAIM_H
#define DIVINUS_CLAIM_H

/* Standalone experimental component. Caller supplies an already generated
 * password hash, never plaintext. 0: set, 1: already claimed, -1: error.
 * Lock and shadow paths must be in trusted, root-owned directories. */
int claim_set_hash(const char *shadow_path, const char *lock_path, const char *hash);

/* 0: empty root password, 1: populated or locked, -1: invalid or unreadable.
 * A positive result is a snapshot, not a guarantee against later changes. */
int claim_state(const char *shadow_path);
#endif
