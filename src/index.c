
#include <stdio.h>
#include <stdlib.h>
#include "lib/verify.h"
#include "lib/util.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define INDEX_URL "https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json"
#define INDEX_SHA256_URL "https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.sha256"
#define INDEX_MINISIG_URL "https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.minisig"
#define DOWNLOAD_DIR "tmp"
#define INDEX_DIR "index"
#define SIG_DIR "sig/index"
#define PUBKEY "ci.pub"

// download, verify, and atomic rewrite
int update_index(void) {
    // download the index file
    char *index_url = INDEX_URL;
    char *index_sha256_url = INDEX_SHA256_URL;
    char *index_minisig_url = INDEX_MINISIG_URL;

    char *index_tmp_path = repman.path_join(DOWNLOAD_DIR, "index.json.tmp");
    char *index_sha256_tmp_path = repman.path_join(DOWNLOAD_DIR, "index.json.sha256.tmp");
    char *index_minisig_tmp_path = repman.path_join(DOWNLOAD_DIR, "index.json.tmp.minisig");

    // rm -rf download_dir
    if (repman.rm(DOWNLOAD_DIR) != 0) {
        fprintf(stderr, "Failed to remove tmp directory: %s\n", DOWNLOAD_DIR);
        free(index_tmp_path);
        free(index_sha256_tmp_path);
        free(index_minisig_tmp_path);
        return -1;
    }

    // create new tmp dir
    if (repman.mkdir_p(DOWNLOAD_DIR) != 0) {
        fprintf(stderr, "Failed to create tmp directory: %s\n", DOWNLOAD_DIR);
        free(index_tmp_path);
        free(index_sha256_tmp_path);
        free(index_minisig_tmp_path);
        return -1;
    }

    int rc = 0;
    if (repman.download(index_url, index_tmp_path) != 0 ||
        repman.download(index_sha256_url, index_sha256_tmp_path) != 0 ||
        repman.download(index_minisig_url, index_minisig_tmp_path) != 0) {
        fprintf(stderr, "Failed to download one or more index artifacts\n");
        rc = -1;
        goto cleanup;
    }

    if (repman.verify_sha256(index_tmp_path, index_sha256_tmp_path) != 0 ||
        repman.verify_minisig(index_tmp_path, index_minisig_tmp_path, PUBKEY) != 0) {
        fprintf(stderr, "Failed to verify checksums or minisign signature for index.json\n");
        rc = -1;
        goto cleanup;
    }
    printf("Verifying checksums and minisign signature for index.json successful\n");

    // atomic rewrite
    char *index_data = repman.read_file(index_tmp_path, NULL);
    char *index_sha256 = repman.read_file(index_sha256_tmp_path, NULL);
    char *index_minisig = repman.read_file(index_minisig_tmp_path, NULL);

    char* index_path = repman.path_join(INDEX_DIR, "index.json");
    char* sha256_path = repman.path_join(SIG_DIR, "index.json.sha256");
    char* sig_path = repman.path_join(SIG_DIR, "index.json.minisig");

    // write index artifacts to disk
    if (repman.write_file(index_path, index_data, strlen(index_data)) != 0 ||
        repman.write_file(sha256_path, index_sha256, strlen(index_sha256)) != 0 ||
        repman.write_file(sig_path, index_minisig, strlen(index_minisig)) != 0) {
            fprintf(stderr, "Failed to write index artifacts to disk\n");
            rc = -1;
    } else {
        printf("Atomic rewrite successful\n");
    }

    // cleanup
    free(index_data);
    free(index_sha256);
    free(index_minisig);
    free(index_path);
    free(sha256_path);
    free(sig_path);

cleanup:
    repman.rm(DOWNLOAD_DIR);
    free(index_tmp_path);
    free(index_sha256_tmp_path);
    free(index_minisig_tmp_path);
    return rc;

}

// gcc src/verify.c src/util.c src/index.c -lcurl -o ./build/index.o
// ./build/index.o

int main() {

    repman.update_index();
    return 0;
}
