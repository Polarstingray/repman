
#include <stdio.h>
#include <stdlib.h>
#include "lib/util.h"
#include "lib/verify.h"
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
int repman_update_index(void) {
    // download the index file
    char *index_url = INDEX_URL;
    char *index_sha256_url = INDEX_SHA256_URL;
    char *index_minisig_url = INDEX_MINISIG_URL;

    char *index_tmp_path = repman_path_join(DOWNLOAD_DIR, "index.json.tmp");
    char *index_sha256_tmp_path = repman_path_join(DOWNLOAD_DIR, "index.json.sha256.tmp");
    char *index_minisig_tmp_path = repman_path_join(DOWNLOAD_DIR, "index.json.tmp.minisig");

    char* index_path = repman_path_join(INDEX_DIR, "index.json");
    char* sha256_path = repman_path_join(SIG_DIR, "index.json.sha256");
    char* sig_path = repman_path_join(SIG_DIR, "index.json.minisig");
    int rc = 0;
    
    // rm -rf download_dir
    if (repman_rm(DOWNLOAD_DIR) != 0) {
        fprintf(stderr, "Failed to remove tmp directory: %s\n", DOWNLOAD_DIR);
        rc = -1;
        goto cleanup;
    }

    // create new tmp dir
    if (repman_mkdir_p(DOWNLOAD_DIR) != 0) {
        fprintf(stderr, "Failed to create tmp directory: %s\n", DOWNLOAD_DIR);
        rc = -1;
        goto cleanup;
    }

    // create sig/index dir
    if (repman_mkdir_p(SIG_DIR) != 0) {
        fprintf(stderr, "Failed to create sig/index directory: %s\n", SIG_DIR);
        rc = -1;
        goto cleanup;
    }

    // download the index artifacts
    if (repman_download(index_url, index_tmp_path) != 0 ||
        repman_download(index_sha256_url, index_sha256_tmp_path) != 0 ||
        repman_download(index_minisig_url, index_minisig_tmp_path) != 0) {
        fprintf(stderr, "Failed to download one or more index artifacts\n");
        rc = -1;
        goto cleanup;
    }
    // verify checksums and minisign signature for index.json
    if (repman_verify_sha256(index_tmp_path, index_sha256_tmp_path) != 0 ||
        repman_verify_minisig(index_tmp_path, index_minisig_tmp_path, PUBKEY) != 0) {
        fprintf(stderr, "Failed to verify checksums or minisign signature for index.json\n");
        rc = -1;
        goto cleanup;
    }
    printf("Verifying checksums and minisign signature for index.json successful\n");

    // atomic rewrite
    rename(index_tmp_path, index_path);
    rename(index_sha256_tmp_path, sha256_path);
    rename(index_minisig_tmp_path, sig_path);
    printf("Atomic rewrite successful\n");

cleanup:
    repman_rm(DOWNLOAD_DIR);
    free(index_path);
    free(sha256_path);
    free(sig_path);
    free(index_tmp_path);
    free(index_sha256_tmp_path);
    free(index_minisig_tmp_path);
    return rc;

}

// 

// gcc src/verify.c src/util.c src/index.c -lcurl -o ./build/index.o
// ./build/index.o

int main() {

    repman_update_index();
    return 0;
}
