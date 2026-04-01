
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


cJSON *repman_parse_json(const char *filepath) {
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

char *get_version(const char *index_path, const char* name, const char* version, const char* os, const char* arch) {
    if (index_path == NULL || name == NULL || os == NULL || arch == NULL) return NULL;

    cJSON *index = repman_parse_json(index_path);
    if (index == NULL) {
        fprintf(stderr, "Failed to parse index\n");
        return NULL;
    }

    cJSON *pkg = cJSON_GetObjectItemCaseSensitive(index, name);
    if (pkg == NULL) {
        // fprintf(stderr, "Package not found: %s_v%s\n", name, version);
        return NULL;
    }

    cJSON *versions = cJSON_GetObjectItemCaseSensitive(pkg, "versions");
    if (versions == NULL) {
        fprintf(stderr, "No versions found for package: %s\n", name);
        return NULL;
    }

    char os_arch[REPMAN_OS_MAX];
    snprintf(os_arch, sizeof(os_arch), "%s_%s", os, arch);

    const char *base_version = version;
    if (base_version == NULL || base_version[0] == '\0') {
        cJSON *latest = cJSON_GetObjectItemCaseSensitive(pkg, "latest");
        if (latest && cJSON_IsString(latest)) {
            base_version = latest->valuestring;
        }
    }

    char *best = NULL;
    for (cJSON *ver = versions->child; ver != NULL; ver = ver->next) {
        if (!cJSON_IsObject(ver)) continue;
        cJSON *targets = cJSON_GetObjectItemCaseSensitive(ver, "targets");
        if (targets == NULL) continue;
        cJSON *target = cJSON_GetObjectItemCaseSensitive(targets, os_arch);
        if (target == NULL) continue;

        const char *ver_str = ver->string;
        if (ver_str == NULL) continue;

        if (base_version != NULL && base_version[0] != '\0') {
            int cmp = cmp_versions(ver_str, base_version);
            if (cmp < 0) continue; /* skip versions older than base */
        }

        if (best == NULL || cmp_versions(ver_str, best) > 0) {
            free(best);
            best = repman_str_dup(ver_str);
        }
    }

    if (best == NULL && base_version != NULL && base_version[0] != '\0') {
        /* Fallback: if no version >= base_version contains os_arch,
         * pick the highest available version that does. */
        for (cJSON *ver = versions->child; ver != NULL; ver = ver->next) {
            if (!cJSON_IsObject(ver)) continue;
            cJSON *targets = cJSON_GetObjectItemCaseSensitive(ver, "targets");
            if (targets == NULL) continue;
            cJSON *target = cJSON_GetObjectItemCaseSensitive(targets, os_arch);
            if (target == NULL) continue;

            const char *ver_str = ver->string;
            if (ver_str == NULL) continue;

            if (best == NULL || cmp_versions(ver_str, best) > 0) {
                free(best);
                best = repman_str_dup(ver_str);
            }
        }
    }

    return best;
}

cJSON *get_pkg(cJSON *pkg, const char* version, const char* os, const char* arch) {
    if (pkg == NULL) return NULL;

    cJSON *versions = cJSON_GetObjectItemCaseSensitive(pkg, "versions");
    if (versions == NULL) return NULL;

    cJSON *ver_obj = cJSON_GetObjectItemCaseSensitive(versions, version);
    if (ver_obj == NULL) return NULL;

    cJSON *targets = cJSON_GetObjectItemCaseSensitive(ver_obj, "targets");
    if (targets == NULL) return NULL;

    char os_arch[REPMAN_OS_MAX];
    snprintf(os_arch, sizeof(os_arch), "%s_%s", os, arch);

    return cJSON_GetObjectItemCaseSensitive(targets, os_arch);
}

char* repman_get_pkg_url(const char *index_path, const char *name, const char* version, const char* os, const char* arch)  {
    if (index_path == NULL || name == NULL || version == NULL || os == NULL || arch == NULL) return NULL;
    cJSON *index = repman_parse_json(index_path);
    if (index == NULL) {
        fprintf(stderr, "Failed to parse index\n");
        return NULL;
    }
    cJSON *pkg = cJSON_GetObjectItemCaseSensitive(index, name);
    if (pkg == NULL) {
        fprintf(stderr, "Package not found in index: %s\n", name);
        cJSON_Delete(index);
        return NULL;
    }
    cJSON *target = get_pkg(pkg, version, os, arch);
    if (target == NULL) {
        fprintf(stderr, "Failed to find package target %s/%s for version %s\n", os, arch, version);
        cJSON_Delete(index);
        return NULL;
    }

    cJSON *url = cJSON_GetObjectItemCaseSensitive(target, "url");
    if (url == NULL) {
        fprintf(stderr, "No URL found for target\n");
        cJSON_Delete(index);
        return NULL;
    }
    char *url_str = repman_str_dup(url->valuestring);
    cJSON_Delete(index);
    return url_str;
}



// download, verify, and atomic rewrite
int repman_update_index(void) {
    // read URLs from environment, fall back to compiled-in defaults
    const char *index_url       = getenv("INDEX_URL");
    const char *index_sha256_url = getenv("INDEX_SHA256_URL");
    const char *index_minisig_url = getenv("INDEX_MINISIG_URL");
    if (index_url       == NULL || index_url[0]       == '\0') index_url       = INDEX_URL;
    if (index_sha256_url == NULL || index_sha256_url[0] == '\0') index_sha256_url = INDEX_SHA256_URL;
    if (index_minisig_url == NULL || index_minisig_url[0] == '\0') index_minisig_url = INDEX_MINISIG_URL;

    // construct paths
    char* data_dir = repman_get_data_dir();
    char* sig_dir = repman_path_join(data_dir, SIG_DIR);

    // char* ipath = repman_path_join(data_dir, INDEX_DIR);

    char* ipath = repman_full_path(INDEX_DIR, INDEX_NAME);
    
    char sig_name[REPMAN_NAME_MAX] = INDEX_NAME;
    strcat(sig_name, ".minisig");
    // char* sig_path = repman_full_path(SIG_DIR, sig_name);
    char* sig_path = repman_path_join(sig_dir, sig_name);

    char sha256_name[REPMAN_NAME_MAX] = INDEX_NAME;
    strcat(sha256_name, ".sha256");
    // char* sha256_path = repman_full_path(sig_dir, sha256_name);
    char* sha256_path = repman_path_join(sig_dir, sha256_name);

    char *pubkey_path = repman_full_path(SIG_DIR, PUBKEY);
    char *download_dir = repman_path_join(data_dir, DOWNLOAD_DIR);
    char index_tmp_name[REPMAN_NAME_MAX] = INDEX_NAME;
    char sha256_tmp_name[REPMAN_NAME_MAX] = INDEX_NAME;
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
        repman_verify_minisig(index_tmp_path, index_minisig_tmp_path, pubkey_path) != 0) {
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
    free(pubkey_path);
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

int repman_update_installed(const char* installed_path, const char *name, const char *version, const char *option) {
    
    if (installed_path == NULL || name == NULL || version == NULL || option == NULL) return -1;

    cJSON *installed = repman_parse_json(installed_path);
    if (installed == NULL) {
        fprintf(stderr, "Failed to parse installed file: %s\n", installed_path);
        installed = cJSON_Parse("{}");
    }
    cJSON *pkg = cJSON_GetObjectItemCaseSensitive(installed, name);
    if ( pkg != NULL ) {
        if (!strcmp(pkg->valuestring, version)) {
            fprintf(stderr, "Already marked as installed: %s\n", name);
            return 1;
        } else {
            printf("updating %s from version: %s to %s\n", name, pkg->valuestring, version);
            cJSON_SetValuestring(pkg, version);
        }
    } else {
        cJSON_AddStringToObject(installed, name, version);
    }
    char *installed_str = cJSON_Print(installed);
    if (repman_write_file(installed_path, installed_str, strlen(installed_str)) != 0) {
        fprintf(stderr, "Failed to write installed file: %s\n", installed_path);
        free(installed_str); cJSON_Delete(installed);
        return -1;
    }
    free(installed_str); cJSON_Delete(installed);
    return 0;
}


char* repman_get_installed_version(const char *filepath, const char *name) {
    cJSON *installed = repman_parse_json(filepath);
    if (installed == NULL) {
        fprintf(stderr, "Failed to parse installed file: %s\n", filepath);
        return NULL;
    }
    if (cJSON_GetObjectItemCaseSensitive(installed, name) == NULL) {
        // fprintf(stderr, "Package not marked installed: %s\n", name);
        return NULL;
    }
    char* installed_version = repman_str_dup(cJSON_GetObjectItemCaseSensitive(installed, name)->valuestring);
    return installed_version;
}

int repman_is_pkg_behind(const char *installed_path, const char *index_path, const char *name, const char *os, const char *arch) {
    char* installed_version = repman_get_installed_version(installed_path, name);
    if (installed_version == NULL) {
        fprintf(stderr, "Failed to get installed version for %s\n", name);
        return -2;
    }

    char *latest_version = get_version(index_path, name, installed_version, os, arch);
    if (latest_version == NULL) {
        fprintf(stderr, "Failed to get latest version for %s\n", name);
        free(installed_version);
        return -2;
    }
    int cmp = cmp_versions(installed_version, latest_version);
    int rc;
    if (cmp < 0) {
        rc = 1;
    } else if (cmp == 0) {
        rc = 0;
    } else {
        rc = -1; // version is ahead?
    }
    free(installed_version);
    free(latest_version);
    return rc;
}

int repman_update_key(void) {
    fprintf(stderr, "repman_update_key: not implemented\n");
    return REPMAN_ERR;
}




// 

// gcc src/verify.c src/util.c src/index.c -lcurl -o ./build/index.o && ./build/index.o && tree ~/.local/share/repman

// int main() {
//     repman_ensure_dirs();
//     // repman_update_index();


//     return 0;
// }
