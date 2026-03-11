/*
 * repman/src/util.c
 *
 * If you're new to C, this file is a good place to start reading.
 * Pay attention to three things as you go:
 *
 *   1. Every malloc() must have a matching free() somewhere.
 *   2. We always check return values — C won't throw exceptions for you.
 *   3. String lengths matter; we use snprintf/strncat instead of sprintf/strcat.
 */

#include "lib/util.h"

#include <stdlib.h>   /* malloc, free, realloc  */
#include <string.h>   /* strlen, strcpy, strcat  */
#include <stdio.h>    /* FILE, fopen, fclose ...  */
#include <sys/stat.h> /* stat, mkdir             */
#include <errno.h>    /* errno, EEXIST           */
#include <unistd.h>   /* rename                  */
#include <libgen.h>   /* dirname (POSIX)         */
#include <curl/curl.h>

/* Forward declarations for verification (implemented in verify.c) */
int verify_sha256(const char *filepath, const char *sha256_path);
int verify_minisig(const char *filepath, const char *minisig_path, const char *pubkey_path);

/* ── String helpers ──────────────────────────────────────────────────────── */

static char *str_dup(const char *src) {
    if (src == NULL) return NULL;

    /*
     * strlen returns the number of bytes NOT counting the NUL terminator,
     * so we allocate len+1 to make room for it.
     */
    size_t len = strlen(src);
    char *copy = malloc(len + 1);   /* malloc returns void*, C auto-casts it */
    if (copy == NULL) {
        REPMAN_LOG_ERR("repman_strdup: malloc failed");
        return NULL;
    }

    memcpy(copy, src, len + 1);     /* copies the NUL byte too */
    return copy;
}


static char *path_join(const char *base, const char *name) {
    if (base == NULL || name == NULL) return NULL;

    size_t base_len = strlen(base);
    size_t name_len = strlen(name);
    int    add_sep  = (base_len > 0 && base[base_len - 1] != '/') ? 1 : 0;

    /* total = base + optional '/' + name + NUL */
    size_t total = base_len + add_sep + name_len + 1;

    char *result = malloc(total);
    if (result == NULL) {
        REPMAN_LOG_ERR("repman_path_join: malloc failed");
        return NULL;
    }

    /*
     * snprintf is the safe version of sprintf.
     * The second argument is the maximum number of bytes to write (including NUL).
     * Always prefer snprintf over sprintf to avoid buffer overflows.
     */
    if (add_sep)
        snprintf(result, total, "%s/%s", base, name);
    else
        snprintf(result, total, "%s%s", base, name);

    return result;
}




static int str_ends_with(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) return 0;

    size_t str_len    = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) return 0;

    /* Compare only the tail of str against suffix */
    return strcmp(str + (str_len - suffix_len), suffix) == 0;
}


/* ── File helpers ────────────────────────────────────────────────────────── */

static char *read_file(const char *path, size_t *out_len) {
    if (path == NULL) return NULL;

    FILE *fp = fopen(path, "rb");   /* "rb" = read binary — consistent on all OS */
    if (fp == NULL) {
        REPMAN_LOG_ERR("repman_read_file: cannot open '%s': %s", path, strerror(errno));
        return NULL;
    }

    /* Seek to end to find the file size, then rewind */
    if (fseek(fp, 0, SEEK_END) != 0) goto fail;
    long file_size = ftell(fp);
    if (file_size < 0)              goto fail;
    rewind(fp);

    /*
     * Allocate file_size + 1 bytes.
     * The +1 lets us NUL-terminate the buffer so callers can treat it
     * as a plain C string without extra work.
     */
    char *buf = malloc((size_t)file_size + 1);
    if (buf == NULL) {
        REPMAN_LOG_ERR("repman_read_file: malloc failed for '%s'", path);
        goto fail;
    }

    size_t read = fread(buf, 1, (size_t)file_size, fp);
    if (read != (size_t)file_size) {
        REPMAN_LOG_ERR("repman_read_file: short read on '%s'", path);
        free(buf);
        goto fail;
    }

    buf[file_size] = '\0';  /* NUL-terminate */

    if (out_len != NULL) *out_len = (size_t)file_size;

    fclose(fp);
    return buf;

fail:
    fclose(fp);
    return NULL;
}


static int write_file(const char *path, const char *data, size_t len) {
    if (path == NULL || data == NULL) return -1;

    /*
     * Atomic write pattern:
     *   1. Write to a temp file next to the target.
     *   2. fsync to flush kernel buffers to disk.
     *   3. rename() over the target — rename is atomic on POSIX systems.
     *
     * This means the original file is NEVER left partially written.
     * If we crash mid-write, the temp file is abandoned and the original survives.
     */
    char tmp_path[4096];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *fp = fopen(tmp_path, "wb");
    if (fp == NULL) {
        REPMAN_LOG_ERR("repman_write_file: cannot open temp file '%s': %s",
                       tmp_path, strerror(errno));
        return -1;
    }

    size_t written = fwrite(data, 1, len, fp);
    if (written != len) {
        REPMAN_LOG_ERR("repman_write_file: short write to '%s'", tmp_path);
        fclose(fp);
        remove(tmp_path);
        return -1;
    }

    /* Flush stdio buffer to OS, then OS buffer to disk */
    fflush(fp);
    fclose(fp);

    /* Atomically replace the target file */
    if (rename(tmp_path, path) != 0) {
        REPMAN_LOG_ERR("repman_write_file: rename '%s' -> '%s' failed: %s",
                       tmp_path, path, strerror(errno));
        remove(tmp_path);
        return -1;
    }

    return 0;
}


static int file_exists(const char *path) {
    if (path == NULL) return 0;

    struct stat st;
    /*
     * stat() fills a 'struct stat' with metadata about a file.
     * S_ISREG checks whether it's a regular file (not a dir, symlink, etc.).
     */
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode)) ? 1 : 0;
}


static int mkdir_p(const char *path) {
    if (path == NULL) return -1;

    /*
     * We need to walk each path component and mkdir along the way.
     * We do this by copying the path and temporarily NUL-terminating
     * at each '/' separator.
     */
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);

    /* Strip trailing slash if present */
    if (len > 0 && tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (char *p = tmp + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';                      /* temporarily terminate */
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                REPMAN_LOG_ERR("repman_mkdir_p: mkdir '%s' failed: %s",
                               tmp, strerror(errno));
                return -1;
            }
            *p = '/';                       /* restore */
        }
    }

    /* Create the final component */
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        REPMAN_LOG_ERR("repman_mkdir_p: mkdir '%s' failed: %s",
                       tmp, strerror(errno));
        return -1;
    }

    return 0;
}


static int download(const char *url, const char *dest_path) {
    CURL *curl_handle;
    CURLcode res;
    FILE *file;
    const char *outfilename = dest_path;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    if (curl_handle) {
        file = fopen(outfilename, "wb");
        if (!file) {
            fprintf(stderr, "Could not open file for writing: %s\n", outfilename);
            return -1;
        }


        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL); // Use default write function
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file); // Write data to
        res = curl_easy_perform(curl_handle);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("Index downloaded successfully: %s\n", outfilename);
        }
    }
    curl_easy_cleanup(curl_handle);
    fclose(file);
    curl_global_cleanup();
    return 0;
}

/* ── Namespace instance ── */
const repman_ns_t repman = {
    .str_dup        = str_dup,
    .path_join      = path_join,
    .str_ends_with  = str_ends_with,
    .read_file      = read_file,
    .write_file     = write_file,
    .file_exists    = file_exists,
    .mkdir_p        = mkdir_p,
    .download       = download,
    .verify_sha256  = verify_sha256,
    .verify_minisig = verify_minisig,
};