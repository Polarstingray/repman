#define _POSIX_C_SOURCE 200809L

#include "lib/verify.h"
#include "lib/util.h"
#include "lib/index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cjson/cJSON.h>

#define PACKAGE_URL "https://github.com/Polarstingray/packages/releases/tag/test-v1.2.10"
#define OS "ubuntu"
#define ARCH "amd64"
#define PUBKEY "ci.pub"
#define INSTALL_JSON "index/installed.json"

/*
 * Example run:
    rm -rf ~/.local/share/repman/packages/*
    gcc src/verify.c src/util.c src/install.c src/index.c -lcjson -lcurl -o ./build/install.o && ./build/install.o && tree ~/.local/share/repman
 */
int check_for_executables(const char *path) {
    DIR *dr;
    struct dirent *de;
    if ((dr = opendir(path)) == NULL) {
        perror("Failed to open directory");
        return -1;
    }
    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        char *full_path = repman_path_join(path, de->d_name);
        if (access(full_path, X_OK) == 0) {
            free(full_path);
            return 0;
        }
        free(full_path);
    }
    closedir(dr);
    printf("No executables found in: %s\n", path);
    return 1;
}

char *repman_resolve_download(const char *url, const char *pkg_and_ver, const char *os, const char *arch, const char *ext) {
    // convert url in index to download url

    // Create base_url by replacing "/tag/" with "/download/" in a duplicated string
    char *base_url = repman_str_repl(repman_str_dup(url), "/tag/", "/download/");
    if (!base_url) return NULL;

    // Build pkg_name: <pkg_and_ver with -v-> _v>_<os>_<arch>.tar.gz
    char *pkg_name_base = repman_str_repl(repman_str_dup(pkg_and_ver), "-v", "_v");
    if (!pkg_name_base) { free(base_url); return NULL; }

    const char *sep = "_";

    size_t need = strlen(pkg_name_base) + strlen(sep) + strlen(os) + strlen(sep) + strlen(arch) + strlen(ext) + 1;
    char *pkg_name = malloc(need);
    if (!pkg_name) {
        free(base_url);
        free(pkg_name_base);
        return NULL;
    }

    snprintf(pkg_name, need, "%s%s%s%s%s%s", pkg_name_base, sep, os, sep, arch, ext);

    char *download_url = repman_path_join(base_url, pkg_name);

    free(base_url);
    free(pkg_name);
    free(pkg_name_base);
    return download_url;
}   

char* repman_pkg_name(const char *pkg_and_ver, const char *os, const char *arch, const char *ext) {
    size_t need = strlen(pkg_and_ver) + strlen(os) + strlen(arch) + strlen(ext) + 3; // 3 for the separator and the trailing NUL
    char *pkg_name = malloc(need);

    // <pkg-ver> –> <pkg_ver>
    char *pkg_ver = repman_str_repl(repman_str_dup(pkg_and_ver), "-v", "_v");

    snprintf(pkg_name, need, "%s_%s_%s%s", pkg_ver, os, arch, ext);
    free(pkg_ver);
    return pkg_name;
}

int repman_extract_tarball(const char *tarball_path, const char *dest_dir) {
    pid_t childpid;
    if ((childpid = fork()) == -1) {
        perror("fork");
        return -1;
    }
    if (childpid == 0) {       /* child process */
        // tar -xzf <tarball> -C ~/.local/share/repman/packages/<pkg_name>_<version>
        printf("tar -xzf %s -C %s\n", tarball_path, dest_dir);
        execlp("tar", "tar", "-xzf", tarball_path, "-C", dest_dir, NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    /* parent process */
    int status;
    waitpid(childpid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "tar extract tarball failed with status %d\n", WEXITSTATUS(status));
        return -1;
    }

    printf("Tarball extracted successfully.\n");
    return 0;
}

static int download_package_files(const char *pkg_url, const char *sig_url, const char *sha256_url, const char *pkg_path, const char *sig_path, const char *sha256_path) {
    if (repman_download(pkg_url, pkg_path) != 0) {
        fprintf(stderr, "Failed to download package\n");
        return -1;
    }
    if (repman_download(sig_url, sig_path) != 0) {
        fprintf(stderr, "Failed to download signature\n");
        return -1;
    }
    if (repman_download(sha256_url, sha256_path) != 0) {
        fprintf(stderr, "Failed to download SHA256\n");
        return -1;
    }
    printf("Download complete.\n");
    return 0;
}

static int verify_package_files(const char *pkg_path, const char *sig_path, const char *sha256_path) {
    repman_verify_minisig(pkg_path, sig_path, PUBKEY);
    repman_verify_sha256(pkg_path, sha256_path);
    printf("Verification Complete.\n");
    return 0;
}

static int check_extracted_package(const char *old_installed_path, const char *bin_dir) {
    if (repman_dir_exists(bin_dir) == 0) {
        fprintf(stderr, "Bin directory not found: %s\n", bin_dir);
        repman_rm(old_installed_path);
        return -1;
    }
    if (check_for_executables(bin_dir) != 0) {
        fprintf(stderr, "Bin directory contains no executables\n");
        repman_rm(old_installed_path);
        return -1;
    }
    return 0;
}


static int symlink_pkg_data(const char *installed_path, const char *local_path, const char* pkg_name) {
    char *new_tmp_dir = malloc(strlen(installed_path) + strlen("/share/") + strlen(pkg_name) + strlen("_tmp") + 1);
    new_tmp_dir = repman_path_join(local_path, "share");
    new_tmp_dir = repman_path_join(new_tmp_dir, pkg_name);
    strcat(new_tmp_dir, "_tmp");
    char *installed_data_dir = repman_path_join(installed_path, "data");

    struct stat st;
    if ((stat(installed_data_dir, &st) != 0) || !S_ISDIR(st.st_mode)){
        perror("stat");
        free(installed_data_dir);
        free(new_tmp_dir);
        return -1;
    }

    if (symlink(installed_data_dir, new_tmp_dir) != 0) {
        perror("symlink");
        fprintf(stderr, "Failed to symlink %s -> %s\n", installed_data_dir, new_tmp_dir);
        free(new_tmp_dir); free(installed_data_dir);
        return -1;
    }
    printf("Symlinked %s -> %s\n", installed_data_dir, new_tmp_dir);

    char *new_data_dir = repman_str_dup(new_tmp_dir);
    repman_str_repl(new_data_dir, "_tmp", ""); 
    if (rename(new_tmp_dir, new_data_dir) != 0) {
        perror("rename");
        fprintf(stderr, "Failed to atomic rewrite %s -> %s\n", new_tmp_dir, new_data_dir);
        free(new_data_dir); free(installed_data_dir); free(new_tmp_dir);
        return -1;
    }
    printf("Renamed %s -> %s\n", new_tmp_dir, new_data_dir);
    free(new_data_dir); free(installed_data_dir); free(new_tmp_dir);
    return 0;
}

static int symlink_pkg_files(const char *new_installed_path, const char *local_path, const char *type) {
    char *new_type_dir = repman_path_join(local_path, type);
    char *installed_type_dir = repman_path_join(new_installed_path, type);
    DIR *dr = opendir(installed_type_dir);
    if (dr == NULL) {
        if (!strcmp(type, "lib")) {
            perror("opendir");
            fprintf(stderr, "Failed to open directory: %s\n", installed_type_dir);
        } else {
            printf("No lib directory in: %s\n", installed_type_dir);
        }
        free(new_type_dir);
        free(installed_type_dir);
        return -2;
    }
    struct dirent *de;
    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        char *installed_type_path = repman_path_join(installed_type_dir, de->d_name);
        char *target_path = repman_path_join(new_type_dir, de->d_name);
        size_t tmp_len = strlen(target_path) + 5;
        char *tmp_path = malloc(tmp_len);
        if (!tmp_path) {
            perror("malloc");
            free(installed_type_path);
            free(target_path);
            closedir(dr);
            free(new_type_dir);
            free(installed_type_dir);
            return -1;
        }
        snprintf(tmp_path, tmp_len, "%s.tmp", target_path);
        struct stat st;

        if ((stat(installed_type_path, &st) != 0) || !S_ISREG(st.st_mode)){
            perror("stat");
            free(installed_type_path);
            free(target_path);
            free(tmp_path);
            continue;
        } 
        if (strcmp(type, "bin") && !((st.st_mode & S_IXUSR) || 
            (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))) {
            fprintf(stderr, "File is not executable: %s\n", installed_type_path);
            free(installed_type_path);
            free(target_path);
            free(tmp_path);
            continue;
        }
        
        if (symlink(installed_type_path, tmp_path) != 0) {
            perror("symlink");
            fprintf(stderr, "Failed to symlink %s -> %s\n", installed_type_path, tmp_path);
            free(tmp_path);
            free(installed_type_path);
            free(target_path);
            closedir(dr);
            free(new_type_dir);
            free(installed_type_dir);
            return -1;
        }
        printf("Symlinked %s -> %s\n", installed_type_path, tmp_path);
        if (rename(tmp_path, target_path) != 0) {
            perror("rename");
            fprintf(stderr, "Failed to atomic rewrite %s -> %s\n", tmp_path, target_path);
            free(tmp_path);
            free(installed_type_path);
            free(target_path);
            closedir(dr);
            free(new_type_dir);
            free(installed_type_dir);
            return -1;
        }
        printf("Renamed %s -> %s\n", tmp_path, target_path);

        free(installed_type_path);
        free(target_path);
        free(tmp_path);
    }
    closedir(dr);
    free(new_type_dir);
    free(installed_type_dir);
    return 0;
}

int repman_download_and_install_pkg(const char *url, const char *pkg_and_ver, const char *os, const char *arch) {
    int rc = 0;
    char* pkg_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz");
    char* sig_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.minisig");
    char* sha256_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.sha256");

    // construct full paths
    char* base_path = repman_get_data_dir();
    char* download_dir = repman_path_join(base_path, "tmp");

    char* pkg_dir = repman_path_join(base_path, "packages");
    char* pkg_ver = repman_str_repl(repman_str_dup(pkg_and_ver), "-v", "_v");
    char* new_installed_path = repman_path_join(pkg_dir, pkg_ver);

    // Extract app_name without modifying pkg_ver
    char* pkg_ver_dup = repman_str_dup(pkg_ver);
    char* v_pos = strstr(pkg_ver_dup, "_v");
    if (v_pos) *v_pos = '\0';
    char* app_name = pkg_ver_dup;
    char* old_installed_path = repman_path_join(pkg_dir, app_name);

    char* bin_dir = NULL;
    char* local_path = NULL;

    char* pkg_path = NULL;
    char* sig_path = NULL;
    char* sha256_path = NULL;

    size_t ver_need = (strlen(pkg_ver) - strlen(app_name) - strlen("_v") + 1);
    char *ver = malloc(ver_need);
    memcpy(ver, pkg_ver + (strlen(app_name) + strlen("_v")), ver_need);

    // check if new_installed_path is a dir already
    if (repman_dir_exists(new_installed_path) != 0) {
        fprintf(stderr, "Package already exists: %s\n", new_installed_path);
        rc = -1;
        goto cleanup;
    }

    bin_dir = repman_path_join(old_installed_path, "bin");
    local_path = repman_get_local_path();

    repman_rm(download_dir);
    if (repman_mkdir_p(download_dir) != 0) {
        fprintf(stderr, "Failed to create tmp directory: %s\n", download_dir);
        rc = -1;
        goto cleanup;
    }

    char* pkg_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz");
    char* sig_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.minisig");
    char* sha256_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.sha256");
    pkg_path = repman_path_join(download_dir, pkg_name);
    sig_path = repman_path_join(download_dir, sig_name);
    sha256_path = repman_path_join(download_dir, sha256_name);
    free(pkg_name); free(sig_name); free(sha256_name);

    if (!pkg_url || !sig_url || !sha256_url) {
        fprintf(stderr, "Failed to resolve download URLs\n");
        rc = -1;
        goto cleanup;
    }

    if (download_package_files(pkg_url, sig_url, sha256_url, pkg_path, sig_path, sha256_path) != 0) {
        rc = -1;
        goto cleanup;
    }

    if (verify_package_files(pkg_path, sig_path, sha256_path) != 0) {
        rc = -1;
        goto cleanup;
    }

    if (repman_extract_tarball(pkg_path, pkg_dir) != 0) {
        fprintf(stderr, "Failed to extract tarball\n");
        rc = -1;
        goto cleanup;
    }

    if (check_extracted_package(old_installed_path, bin_dir) != 0) {
        rc = -1;
        goto cleanup;
    }
    repman_rm(download_dir);

    printf("renaming %s -> %s\n", old_installed_path, new_installed_path);
    if (rename(old_installed_path, new_installed_path) != 0) {
        perror("rename");
        fprintf(stderr, "Failed to rename package\n");
        rc = -1;
        repman_rm(old_installed_path);
        goto cleanup;
    }
    
    if (symlink_pkg_files(new_installed_path, local_path, "bin") != 0) {
        rc = -1;
        goto cleanup;
    }
    int lib_rc = symlink_pkg_files(new_installed_path, local_path, "lib");
    if (lib_rc != 0 && lib_rc != -2) {
        rc = -1;
        goto cleanup;
    }

    if (symlink_pkg_data(new_installed_path, local_path, app_name) != 0) {
        rc = -1;
        goto cleanup;
    }

    int installed_rc = repman_update_installed(INSTALL_JSON, app_name, ver, "install");
    if (installed_rc != 0 && installed_rc != 1) {
        fprintf(stderr, "Failed to update installed.json\n");
        rc = -1;
        goto cleanup;
    }

    printf("Package installed successfully.\n");
    

cleanup:
    free(ver);
    free(local_path);
    free(bin_dir);
    free(old_installed_path);
    free(app_name);
    free(new_installed_path);
    free(pkg_ver);
    free(pkg_dir);
    free(download_dir);
    free(base_path);
    free(pkg_url);
    free(sig_url);
    free(sha256_url);
    free(pkg_path);
    free(sig_path);
    free(sha256_path);
    return rc;
}


#ifndef TESTING
int main() {
    // char *url = repman_resolve_download(PACKAGE_URL, "test_v1.2.10", "ubuntu", "amd64", ".tar.gz");
    // printf("Download URL: %s\n", url);

    // printf("Downloading...");
    // char* pkg_name = repman_pkg_name("test-v1.2.10", "ubuntu", "amd64", ".tar.gz");
    // printf("Package Name: %s\n", pkg_name);
    // repman_download(url, pkg_name);
    // printf("Download complete.\n");
    // free(url); free(pkg_name);

    repman_ensure_dirs();

    // char* pkg_and_ver = "test-v1.2.10";
    char* pkg_name = "affirm";
    char* pkg_ver = "1.0.0";
    char* os = "ubuntu";
    char* arch = "amd64";

    char *index_path = repman_full_path("index", "index.json");
    cJSON *index = repman_parse_json(index_path);
    if (index == NULL) {
        fprintf(stderr, "Failed to parse index\n");
        return -1;
    }
    
    char *resolved_version = get_version(index_path, pkg_name, pkg_ver, os, arch);
    free(index_path);
    if (resolved_version == NULL) {
        fprintf(stderr, "Failed to determine version for package: %s\n", pkg_name);
        cJSON_Delete(index);
        return -1;
    }

    char pkg_and_ver[32];
    sprintf(pkg_and_ver, "%s-v%s", pkg_name, resolved_version);

    cJSON *pkg = cJSON_GetObjectItemCaseSensitive(index, pkg_name);
    cJSON *target = get_pkg(pkg, resolved_version, os, arch);
    if (target == NULL) {
        fprintf(stderr, "Failed to find package target %s/%s for version %s\n", os, arch, resolved_version);
        free(resolved_version);
        cJSON_Delete(index);
        return -1;
    }

    char *pkg_url = repman_get_pkg_url(target);
    free(resolved_version);
    if (pkg_url == NULL) {
        fprintf(stderr, "Failed to get package URL\n");
        cJSON_Delete(index);
        return -1;
    }

    int rc = repman_download_and_install_pkg(pkg_url, pkg_and_ver, os, arch);

    free(pkg_url);
    cJSON_Delete(index);

    return rc;
}
#endif

// https://github.com/Polarstingray/packages/releases/download/test-v1.1.10/test_v1.2.10_ubuntu_amd64.tar.gz
// 