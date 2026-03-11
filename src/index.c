
#include <stdio.h>
#include <stdlib.h>
#include "lib/verify.h"
#include "lib/util.h"

#define INDEX_URL "https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json"
#define INDEX_SHA256_URL "https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.sha256"
#define INDEX_MINISIG_URL "https://raw.githubusercontent.com/Polarstingray/packages/refs/heads/main/index/index.json.minisig"
#define DOWNLOAD_DIR "tmp/"


// download, verify, and atomic rewrite
int update_index() {
    // download the index file
    char *index_url = INDEX_URL;
    char *index_sha256_url = INDEX_SHA256_URL;
    char *index_minisig_url = INDEX_MINISIG_URL;

    char *index_tmp_path = repman.path_join(DOWNLOAD_DIR, "index.json.tmp");
    char *index_sha256_tmp_path = repman.path_join(DOWNLOAD_DIR, "index.json.sha256.tmp");
    char *index_minisig_tmp_path = repman.path_join(DOWNLOAD_DIR, "index.json.tmp.minisig");

    // rm tmp dir and all its contents

    // create new tmp dir
    if (repman.mkdir_p(DOWNLOAD_DIR) != 0) {
        fprintf(stderr, "Failed to create tmp directory: %s\n", DOWNLOAD_DIR);
        return -1;
    }


    repman.download(index_url, index_tmp_path);
    repman.download(index_sha256_url, index_sha256_tmp_path);
    repman.download(index_minisig_url, index_minisig_tmp_path);
    if (verify_sha256(index_tmp_path, index_sha256_tmp_path) != 0 || verify_minisig(index_tmp_path, index_minisig_tmp_path, "ci.pub") != 0) {
        fprintf(stderr, "Failed to verify checksums or minisign signature for index.json\n");
        return -1;
    }
    printf("Verifying checksums and minisign signature for index.json successful\n");
    return 0;
}



// gcc src/verify.c src/util.c src/index.c -lcurl -o ./build/index.o
// ./build/index.o

int main() {

    update_index();
    return 0;
}
