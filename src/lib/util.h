#ifndef REPMAN_UTIL_H
#define REPMAN_UTIL_H

#include <stddef.h>  /* size_t */
#include <stdio.h>   /* FILE  */

/* ─────────────────────────────────────────────
 * repman/src/util.h
 *
 * Public API surface for repman as a C "namespace".
 * We expose generic function names via a single global const struct
 * named `repman`. Callers use `repman.path_join(...)`, etc.
 *
 * Implementation details remain internal to the .c files. This avoids
 * polluting the global symbol space with long repman_* prefixes while
 * still preventing name collisions in host programs.
 * ───────────────────────────────────────────── */

/* ── Error / logging helpers ─────────────────────────────────────────────── */
#define REPMAN_LOG_INFO(fmt, ...) \
    fprintf(stderr, "[repman] " fmt "\n", ##__VA_ARGS__)

#define REPMAN_LOG_ERR(fmt, ...) \
    fprintf(stderr, "[repman][ERROR] " fmt "\n", ##__VA_ARGS__)

/* ── Namespace: repman ───────────────────────────────────────────────────── */

typedef struct repman_ns_s {
    /* String helpers */
    char *(*str_dup)(const char *src);
    char *(*path_join)(const char *base, const char *name);
    int   (*str_ends_with)(const char *str, const char *suffix);

    /* File helpers */
    char *(*read_file)(const char *path, size_t *out_len);
    int   (*write_file)(const char *path, const char *data, size_t len);
    int   (*file_exists)(const char *path);
    int   (*mkdir_p)(const char *path);
    int   (*rm)(const char *path);   /* recursive remove: files or directories */

    /* Network helpers */
    int   (*download)(const char *url, const char *dest_path);

    /* Verification helpers */
    int   (*verify_sha256)(const char *filepath, const char *sha256_path);
    int   (*verify_minisig)(const char *filepath, const char *minisig_path, const char *pubkey_path);

    int   (*update_index)();
} repman_ns_t;

/* Global namespace instance */
extern const repman_ns_t repman;

#endif /* REPMAN_UTIL_H */