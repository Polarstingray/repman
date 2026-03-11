


#include "lib/util.h"
#include "lib/index.h"
#include <stdio.h>
#include <curl/curl.h>
#include "lib/verify.h"
#include <string.h>


int download(const char *url, const char *dest_path) {
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
            return 1;
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


// int main() {
//     download("https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json", "index.json");
//     download("https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.sha256", "index.json.sha256");
//     download("https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.minisig", "index.json.minisig");
//     return 0;
// }
