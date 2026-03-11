#ifndef REPMAN_UTIL_H
#define REPMAN_UTIL_H

#include <stddef.h>  /* size_t */
#include <stdio.h>   /* FILE  */

/* ─────────────────────────────────────────────
 * repman/src/util.h
 *
 * Shared helpers used by every other module.
 * Keep everything here small and self-contained —
 * if a helper only makes sense for one module,
 * put it there instead.
 * ───────────────────────────────────────────── */


/* ── String helpers ──────────────────────────────────────────────────────── */

/*
 * repman_strdup  —  duplicate a string onto the heap.
 *
 * Why not just use strdup()?  strdup() is POSIX, not standard C.
 * This wrapper also lets us centralise error handling later.
 *
 * Caller is responsible for calling free() on the returned pointer.
 * Returns NULL on allocation failure.
 */
char *repman_strdup(const char *src);

/*
 * repman_path_join  —  join two path components with a '/'.
 *
 *   repman_path_join("/var/lib/repman", "index.json")
 *   => "/var/lib/repman/index.json"   (caller must free)
 *
 * Returns NULL on allocation failure.
 */
char *repman_path_join(const char *base, const char *name);

/*
 * repman_str_ends_with  —  return 1 if 'str' ends with 'suffix', else 0.
 */
int repman_str_ends_with(const char *str, const char *suffix);


/* ── File helpers ────────────────────────────────────────────────────────── */

/*
 * repman_read_file  —  read an entire file into a heap-allocated buffer.
 *
 * Writes the number of bytes read into *out_len if out_len is not NULL.
 * The returned buffer is NUL-terminated so it can be treated as a string.
 * Caller must free() the result.
 * Returns NULL on error.
 */
char *repman_read_file(const char *path, size_t *out_len);

/*
 * repman_write_file  —  atomically write data to path.
 *
 * Writes to a temporary file first, then renames into place so the
 * original is never left half-written on crash.
 * Returns 0 on success, -1 on error.
 */
int repman_write_file(const char *path, const char *data, size_t len);

/*
 * repman_file_exists  —  return 1 if path exists and is a regular file.
 */
int repman_file_exists(const char *path);

/*
 * repman_mkdir_p  —  create directory (and any missing parents).
 * Like `mkdir -p`.  Returns 0 on success, -1 on error.
 */
int repman_mkdir_p(const char *path);

/*
* repman_download  —  download a file from a URL to a destination path.
* Returns 0 on success, -1 on error.
*/
int repman_download(const char *url, const char *dest_path);


/* ── Error / logging helpers ─────────────────────────────────────────────── */

/*
 * Logging macros.
 *
 * Usage:  REPMAN_LOG_INFO("loaded %d packages", count);
 *         REPMAN_LOG_ERR("failed to open %s", path);
 *
 * All output goes to stderr so it doesn't pollute stdout
 * (which Python may parse).
 */
#define REPMAN_LOG_INFO(fmt, ...) \
    fprintf(stderr, "[repman] " fmt "\n", ##__VA_ARGS__)

#define REPMAN_LOG_ERR(fmt, ...) \
    fprintf(stderr, "[repman][ERROR] " fmt "\n", ##__VA_ARGS__)


#endif /* REPMAN_UTIL_H */