#ifndef REPMAN_UTIL_H
#define REPMAN_UTIL_H

#include <stddef.h>  /* size_t */
#include <stdio.h>   /* FILE  */
#include "repman.h"

/* ─────────────────────────────────────────────
 * repman/src/util.h
 *
 * Public API surface for repman as global functions prefixed with repman_*.
 * This avoids exposing a struct-based namespace. Callers use repman_*()
 * directly, e.g. repman_path_join(...), repman_mkdir_p(...), etc.
 * ───────────────────────────────────────────── */

/* ── Error / logging helpers ─────────────────────────────────────────────── */
#define REPMAN_LOG_INFO(...) \
    fprintf(stderr, "[repman] " __VA_ARGS__)

#define REPMAN_LOG_ERR(...) \
    fprintf(stderr, "[repman][ERROR] " __VA_ARGS__)

/* ── API: repman_* functions ─────────────────────────────────────────────── */

/* String helpers */
char *repman_str_dup(const char *src);
char *repman_path_join(const char *base, const char *name);
char *repman_str_repl(char *s, const char *s1, const char *s2);
int   repman_str_ends_with(const char *str, const char *suffix);

/* File helpers */
char *repman_read_file(const char *path, size_t *out_len);
int   repman_write_file(const char *path, const char *data, size_t len);
int   repman_file_exists(const char *path);
int   repman_dir_exists(const char *path);
int   repman_mkdir_p(const char *path);
int   repman_rm(const char *path);   /* recursive remove: files or directories */
char *repman_get_data_dir(void);
char *repman_get_local_path(void);
void  repman_ensure_dirs(void);


/* Network helpers — implemented in net.c, re-exported here for convenience */
#include "net.h"

#endif /* REPMAN_UTIL_H */
