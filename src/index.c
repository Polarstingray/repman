
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
#define INDEX_NAME "index.json"
#define SIG_DIR "sig/index"
#define PUBKEY "ci.pub"

// construct path
char* repman_full_path(const char *dir_, const char *name) {
    char* base = repman_get_data_dir();
    char* dir = repman_path_join(base, dir_);
    char* path = repman_path_join(dir, name);
    free(base); free(dir);
    return path;
}

// download, verify, and atomic rewrite
int repman_update_index(void) {
    // download the index file
    char *index_url = INDEX_URL;
    char *index_sha256_url = INDEX_SHA256_URL;
    char *index_minisig_url = INDEX_MINISIG_URL;

    // construct paths
    char* ipath = repman_full_path(INDEX_DIR, INDEX_NAME);
    
    char* sig_dir = repman_path_join(repman_get_data_dir(), SIG_DIR);
    
    char sig_name[32] = INDEX_NAME;
    strcat(sig_name, ".minisig");
    char* sig_path = repman_full_path(sig_dir, sig_name);

    char sha256_name[32] = INDEX_NAME;
    strcat(sha256_name, ".sha256");
    char* sha256_path = repman_full_path(sig_dir, sha256_name);

    char *download_dir = repman_path_join(repman_get_data_dir(), DOWNLOAD_DIR);
    char index_tmp_name[32] = INDEX_NAME;
    char sha256_tmp_name[32] = INDEX_NAME;
    strcat(index_tmp_name, ".tmp");
    char *sig_tmp_name = repman_str_dup(index_tmp_name);
    
    strcat(sha256_tmp_name, ".tmp.sha256");
    strcat(sig_tmp_name, ".minisig");
    char *index_tmp_path = repman_path_join(download_dir, index_tmp_name);
    char *index_sha256_tmp_path = repman_path_join(download_dir, sha256_tmp_name);
    char *index_minisig_tmp_path = repman_path_join(download_dir, sig_tmp_name);
    free(sig_tmp_name);

    int rc = 0;
    // rm -rf download_dir
    if (repman_rm(download_dir) != 0) {
        fprintf(stderr, "Failed to remove tmp directory: %s\n", DOWNLOAD_DIR);
        rc = -1;
        goto cleanup;
    }
    // create new tmp dir
    if (repman_mkdir_p(download_dir) != 0) {
        fprintf(stderr, "Failed to create tmp directory: %s\n", DOWNLOAD_DIR);
        rc = -1;
        goto cleanup;
    }
    // create sig/index dir
    if (repman_mkdir_p(sig_dir) != 0) {
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
    rename(index_tmp_path, ipath);
    rename(index_sha256_tmp_path, sha256_path);
    rename(index_minisig_tmp_path, sig_path);
    printf("Atomic rewrite successful\n");

cleanup:
    repman_rm(download_dir);
    free(download_dir);
    free(ipath);
    free(sha256_path);
    free(sig_path);
    free(index_tmp_path);
    free(index_sha256_tmp_path);
    free(index_minisig_tmp_path);
    return rc;
}

// 

// gcc src/verify.c src/util.c src/index.c -lcurl -o ./build/index.o && ./build/index.o && tree ~/.local/share/repman

int main() {
    repman_ensure_dirs();
    repman_update_index();
    return 0;
}
