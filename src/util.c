

#include "lib/util.h"

#include <stdlib.h>   /* malloc, free, realloc  */
#include <string.h>   /* strlen, strcpy, strcat  */
#include <stdio.h>    /* FILE, fopen, fclose ...  */
#include <sys/stat.h> /* stat, mkdir             */
#include <errno.h>    /* errno, EEXIST           */
#include <unistd.h>   /* rename                  */
#include <libgen.h>   /* dirname (POSIX)         */
#include <curl/curl.h>
#include <dirent.h>  /* DIR, opendir, readdir */
#include <pwd.h>


/* ── String helpers ──────────────────────────────────────────────────────── */

char *repman_str_dup(const char *src) {
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

/*
* repman_str_repl: replace 1st occurrence of substring with another.
*/
char *repman_str_repl(char *s, const char *s1, const char *s2) {
    if (s == NULL || s1 == NULL || s2 == NULL) return s;

    char *p = strstr(s, s1); // Find the first occurrence of s1
    if (p == NULL) return s; // nothing to replace

    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    if (len1 == len2) {
        memcpy(p, s2, len2);
        return s;
    }

    // We may need to grow or shrink the buffer. Compute new total length.
    size_t orig_len = strlen(s);
    size_t tail_len = strlen(p + len1); // part after s1
    size_t new_total = orig_len - len1 + len2 + 1; // +1 for NUL

    // Preserve offset before reallocation
    size_t off = (size_t)(p - s);

    char *ns = realloc(s, new_total);
    if (ns == NULL) {
        // Allocation failed; leave original unmodified
        REPMAN_LOG_ERR("repman_str_repl: realloc failed");
        return s;
    }

    // Recompute 'p' in case realloc moved the buffer
    p = ns + off;

    // Shift tail to make room (or close the gap) and insert replacement
    memmove(p + len2, p + len1, tail_len + 1); // +1 to move the NUL
    memcpy(p, s2, len2);

    return ns;
}


char *repman_path_join(const char *base, const char *name) {
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




int repman_str_ends_with(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) return 0;

    size_t str_len    = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) return 0;

    /* Compare only the tail of str against suffix */
    return strcmp(str + (str_len - suffix_len), suffix) == 0;
}


/* ── File helpers ────────────────────────────────────────────────────────── */

char *repman_read_file(const char *path, size_t *out_len) {
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


int repman_write_file(const char *path, const char *data, size_t len) {
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


int repman_file_exists(const char *path) {
    if (path == NULL) return 0;

    struct stat st;
    /*
     * stat() fills a 'struct stat' with metadata about a file.
     * S_ISREG checks whether it's a regular file (not a dir, symlink, etc.).
     */
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode)) ? 1 : 0;
}


int repman_mkdir_p(const char *path) {
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


int repman_rm(const char* path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        /* Nothing to remove */
        return 0;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) {
            REPMAN_LOG_ERR("rm_rf: cannot open dir '%s': %s", path, strerror(errno));
            return -1;
        }
        struct dirent *entry;
        while ((entry = readdir(d)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char *full_path = repman_path_join(path, entry->d_name);
            if (full_path == NULL) {
                REPMAN_LOG_ERR("rm_rf: OOM building path for '%s/%s'", path, entry->d_name);
                closedir(d);
                return -1;
            }
            if (repman_rm(full_path) != 0) {
                free(full_path);
                closedir(d);
                return -1;
            }
            free(full_path);
        }
        closedir(d);
        if (rmdir(path) != 0) {
            REPMAN_LOG_ERR("rm_rf: rmdir '%s' failed: %s", path, strerror(errno));
            return -1;
        }
        return 0;
    }

    /* Regular file or symlink */
    if (unlink(path) != 0) {
        REPMAN_LOG_ERR("rm_rf: unlink '%s' failed: %s", path, strerror(errno));
        return -1;
    }
    return 0;
}

int repman_download(const char *url, const char *dest_path) {
    CURL *curl_handle = NULL;
    CURLcode res = CURLE_OK;
    FILE *file = NULL;
    const char *outfilename = dest_path;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    if (!curl_handle) {
        fprintf(stderr, "curl_easy_init() failed\n");
        curl_global_cleanup();
        return -1;
    }

    file = fopen(outfilename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file for writing: %s\n", outfilename);
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        return -1;
    }

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL); // Use default write function
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file); // Write data to
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("Index downloaded successfully: %s\n", outfilename);
    }

    curl_easy_cleanup(curl_handle);
    if (file) fclose(file);
    curl_global_cleanup();
    return (res == CURLE_OK) ? 0 : -1;
}

char *repman_get_data_dir(void) {
    const char *xdg = getenv("XDG_DATA_HOME");
    char *base;
    if (xdg != NULL && xdg[0] != '\0') {
        base = repman_path_join(xdg, "repman");
    } else {
        const char *home = getenv("HOME");
        if (home == NULL) {
            home = getpwuid(getuid())->pw_dir; 
        }
        base = repman_path_join(home, ".local/share/repman");
    }
    return base;
}

void repman_ensure_dirs(void) {
    char *base  = repman_get_data_dir();
    char *index = repman_path_join(base, "index");
    char *sig   = repman_path_join(base, "sig/index");
    char *tmp   = repman_path_join(base, "tmp");
    char *cache = repman_path_join(base, "cache");

    repman_mkdir_p(index);
    repman_mkdir_p(sig);
    repman_mkdir_p(tmp);
    repman_mkdir_p(cache);

    free(base); free(index); free(sig); free(tmp); free(cache);
}