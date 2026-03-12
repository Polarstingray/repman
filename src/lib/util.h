#ifndef REPMAN_UTIL_H
#define REPMAN_UTIL_H

#include <stddef.h>  /* size_t */
#include <stdio.h>   /* FILE  */

/* ─────────────────────────────────────────────
 * repman/src/util.h
 *
 * Public API surface for repman as global functions prefixed with repman_*.
 * This avoids exposing a struct-based namespace. Callers use repman_*()
 * directly, e.g. repman_path_join(...), repman_mkdir_p(...), etc.
 * ───────────────────────────────────────────── */

/* ── Error / logging helpers ─────────────────────────────────────────────── */
#define REPMAN_LOG_INFO(fmt, ...) \
    fprintf(stderr, "[repman] " fmt "\n", ##__VA_ARGS__)

#define REPMAN_LOG_ERR(fmt, ...) \
    fprintf(stderr, "[repman][ERROR] " fmt "\n", ##__VA_ARGS__)

/* ── API: repman_* functions ─────────────────────────────────────────────── */

/* String helpers */
char *repman_str_dup(const char *src);
char *repman_path_join(const char *base, const char *name);
int   repman_str_ends_with(const char *str, const char *suffix);

/* File helpers */
char *repman_read_file(const char *path, size_t *out_len);
int   repman_write_file(const char *path, const char *data, size_t len);
int   repman_file_exists(const char *path);
int   repman_mkdir_p(const char *path);
int   repman_rm(const char *path);   /* recursive remove: files or directories */

/* Network helpers */
int   repman_download(const char *url, const char *dest_path);

#endif /* REPMAN_UTIL_H */
