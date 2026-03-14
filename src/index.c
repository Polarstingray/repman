
#include <stdio.h>
#include <stdlib.h>
#include "lib/util.h"
#include "lib/verify.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <cjson/cJSON.h>

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


cJSON *repman_parse_index(const char *filepath) {
    char* buf = repman_read_file(filepath, NULL);
    if (buf == NULL) {
        fprintf(stderr, "Failed to read file: %s\n", filepath);
        return NULL;
    }
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        free(buf);
        return NULL;
    }
    free(buf);
    return root;
}

cJSON *get_pkg(cJSON *index, const char* name, const char* version, const char* os, const char* arch) {
    cJSON *pkg = cJSON_GetObjectItemCaseSensitive(index, name);
    if (pkg == NULL) {
        fprintf(stderr, "Package not found: %s\n", name);
        return NULL;
    }

    const char *ver_to_use = version;
    if (ver_to_use == NULL || strlen(ver_to_use) == 0) {
        cJSON *latest = cJSON_GetObjectItemCaseSensitive(pkg, "latest");
        if (latest && cJSON_IsString(latest)) {
            ver_to_use = latest->valuestring;
        } else {
            fprintf(stderr, "No version specified and no latest version found for package: %s\n", name);
            return NULL;
        }
    }

    cJSON *versions = cJSON_GetObjectItemCaseSensitive(pkg, "versions");
    if (versions == NULL) {
        fprintf(stderr, "No versions found for package: %s\n", name);
        return NULL;
    }

    cJSON *ver_obj = cJSON_GetObjectItemCaseSensitive(versions, ver_to_use);
    if (ver_obj == NULL) {
        fprintf(stderr, "Version %s not found for package: %s\n", ver_to_use, name);
        return NULL;
    }

    cJSON *targets = cJSON_GetObjectItemCaseSensitive(ver_obj, "targets");
    if (targets == NULL) {
        fprintf(stderr, "No targets found for version %s of package: %s\n", ver_to_use, name);
        return NULL;
    }

    char os_arch[256];
    snprintf(os_arch, sizeof(os_arch), "%s_%s", os, arch);

    cJSON *target = cJSON_GetObjectItemCaseSensitive(targets, os_arch);
    if (target == NULL) {
        fprintf(stderr, "Target %s not found for version %s of package: %s\n", os_arch, ver_to_use, name);
        return NULL;
    }

    return target;
}

char* repman_get_pkg_url(cJSON *target)  {
    cJSON *url = cJSON_GetObjectItemCaseSensitive(target, "url");
    if (url == NULL) {
        fprintf(stderr, "No URL found for target\n");
        return NULL;
    }
    return repman_str_dup(url->valuestring);
}

int cmp_versions(const char *a, const char *b) {
    int maj1, min1, pat1, maj2, min2, pat2;

    if (sscanf(a, "%d.%d.%d", &maj1, &min1, &pat1) != 3) {
        fprintf(stderr, "Invalid version format: %s\n", a);
        return -2;
    }

    if (sscanf(b, "%d.%d.%d", &maj2, &min2, &pat2) != 3) {
        fprintf(stderr, "Invalid version format: %s\n", b);
        return -2;
    }
    if (maj1 > maj2) return 1;
    if (maj1 < maj2) return -1;

    if (min1 > min2) return 1;
    if (min1 < min2) return -1;
    
    if (pat1 > pat2) return 1;
    if (pat1 < pat2) return -1;
    return 0;
}


// download, verify, and atomic rewrite
int repman_update_index(void) {
    // download the index file
    char *index_url = INDEX_URL;
    char *index_sha256_url = INDEX_SHA256_URL;
    char *index_minisig_url = INDEX_MINISIG_URL;

    // construct paths
    char* data_dir = repman_get_data_dir();
    char* sig_dir = repman_path_join(data_dir, SIG_DIR);

    // char* ipath = repman_path_join(data_dir, INDEX_DIR);

    char* ipath = repman_full_path(INDEX_DIR, INDEX_NAME);
    
    char sig_name[32] = INDEX_NAME;
    strcat(sig_name, ".minisig");
    // char* sig_path = repman_full_path(SIG_DIR, sig_name);
    char* sig_path = repman_path_join(sig_dir, sig_name);

    char sha256_name[32] = INDEX_NAME;
    strcat(sha256_name, ".sha256");
    // char* sha256_path = repman_full_path(sig_dir, sha256_name);
    char* sha256_path = repman_path_join(sig_dir, sha256_name);

    char *download_dir = repman_path_join(data_dir, DOWNLOAD_DIR);
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
    if (rename(index_tmp_path, ipath) != 0) {
        fprintf(stderr, "Failed to atomic rewrite index\n");
        rc = -1;
        goto cleanup;
    }
    if (rename(index_sha256_tmp_path, sha256_path) != 0) {
        fprintf(stderr, "Failed to atomic rewrite sha256 for index\n");
        rc = -1;
        goto cleanup;
    }
    if (rename(index_minisig_tmp_path, sig_path) != 0) {
        fprintf(stderr, "Failed to atomic rewrite signature for index\n");
        rc = -1;
        goto cleanup;
    }
    printf("Atomic rewrite successful\n");

cleanup:
    repman_rm(download_dir);
    free(download_dir);
    free(ipath);
    free(sha256_path);
    free(sig_path);
    free(sig_dir);
    free(index_tmp_path);
    free(index_sha256_tmp_path);
    free(index_minisig_tmp_path);
    free(data_dir);
    return rc;
}

// 

// gcc src/verify.c src/util.c src/index.c -lcurl -o ./build/index.o && ./build/index.o && tree ~/.local/share/repman

// int main() {
//     repman_ensure_dirs();
//     // repman_update_index();


//     return 0;
// }
