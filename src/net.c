#include "lib/net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "lib/repman.h"

/* One-time curl global initialisation.  curl_global_cleanup() is registered
 * with atexit() so it runs exactly once at program exit. */
static void repman_net_init(void) {
    static int done = 0;
    if (!done) {
        curl_global_init(CURL_GLOBAL_ALL);
        atexit(curl_global_cleanup);
        done = 1;
    }
}

int repman_download(const char *url, const char *dest_path) {
    repman_net_init();

    CURL *curl_handle = NULL;
    CURLcode res = CURLE_OK;
    FILE *file = NULL;
    const char *outfilename = dest_path;

    curl_handle = curl_easy_init();

    if (!curl_handle) {
        fprintf(stderr, "curl_easy_init() failed\n");
        return -1;
    }

    file = fopen(outfilename, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file for writing: %s\n", outfilename);
        curl_easy_cleanup(curl_handle);
        return -1;
    }

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("downloaded successfully: %s\n", outfilename);
    }

    curl_easy_cleanup(curl_handle);
    if (file) fclose(file);
    return (res == CURLE_OK) ? 0 : -1;
}

int repman_download_atomic(const char *url, const char *dest_path) {
    char tmp_path[REPMAN_PATH_MAX];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", dest_path);

    int rc = repman_download(url, tmp_path);
    if (rc != 0) {
        remove(tmp_path);
        return REPMAN_ERR;
    }

    if (rename(tmp_path, dest_path) != 0) {
        fprintf(stderr, "repman_download_atomic: rename '%s' -> '%s' failed\n",
                tmp_path, dest_path);
        remove(tmp_path);
        return REPMAN_ERR;
    }

    return REPMAN_OK;
}
