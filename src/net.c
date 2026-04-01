#include "lib/net.h"

#include <stdio.h>
#include <curl/curl.h>

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
        printf("downloaded successfully: %s\n", outfilename);
    }

    curl_easy_cleanup(curl_handle);
    if (file) fclose(file);
    curl_global_cleanup();
    return (res == CURLE_OK) ? 0 : -1;
}
