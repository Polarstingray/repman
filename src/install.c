

#include "lib/verify.h"
#include "lib/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define PACKAGE_URL "https://github.com/Polarstingray/packages/releases/tag/test-v1.2.10"
#define OS "ubuntu"
#define ARCH "amd64"
#define PUBKEY "ci.pub"

// gcc src/verify.c src/util.c src/install.c src/index.c -lcurl -o ./build/install.o && ./build/install.o && tree ~/.local/share/repman
// rm -rf ~/.local/share/repman/packages/*
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
    if (!repman_dir_exists(bin_dir)) {
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

static int symlink_package_executables(const char *new_installed_path, const char *local_path) {
    char *new_bin_dir = repman_path_join(local_path, "bin");
    char *installed_bin_dir = repman_path_join(new_installed_path, "bin");
    DIR *dr = opendir(installed_bin_dir);
    if (dr == NULL) {
        perror("opendir");
        fprintf(stderr, "Failed to open directory: %s\n", installed_bin_dir);
        free(new_bin_dir);
        free(installed_bin_dir);
        return -1;
    }
    struct dirent *de;
    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
        char *installed_bin_path = repman_path_join(installed_bin_dir, de->d_name);
        char *target_path = repman_path_join(new_bin_dir, de->d_name);
        size_t tmp_len = strlen(target_path) + 5;
        char *tmp_path = malloc(tmp_len);
        if (!tmp_path) {
            perror("malloc");
            free(installed_bin_path);
            free(target_path);
            closedir(dr);
            free(new_bin_dir);
            free(installed_bin_dir);
            return -1;
        }
        snprintf(tmp_path, tmp_len, "%s.tmp", target_path);
        struct stat st;
        if (stat(installed_bin_path, &st) == 0 && S_ISREG(st.st_mode)) {
            if (symlink(installed_bin_path, tmp_path) != 0) {
                perror("symlink");
                fprintf(stderr, "Failed to symlink %s -> %s\n", installed_bin_path, tmp_path);
                free(tmp_path);
                free(installed_bin_path);
                free(target_path);
                closedir(dr);
                free(new_bin_dir);
                free(installed_bin_dir);
                return -1;
            }
            printf("Symlinked %s -> %s\n", installed_bin_path, tmp_path);
            if (rename(tmp_path, target_path) != 0) {
                perror("rename");
                fprintf(stderr, "Failed to atomic rewrite %s -> %s\n", tmp_path, target_path);
                free(tmp_path);
                free(installed_bin_path);
                free(target_path);
                closedir(dr);
                free(new_bin_dir);
                free(installed_bin_dir);
                return -1;
            }
            printf("Renamed %s -> %s\n", tmp_path, target_path);
        } else {
            printf("Not executable: %s\n", installed_bin_path);
        }
        free(installed_bin_path);
        free(target_path);
        free(tmp_path);
    }
    closedir(dr);
    free(new_bin_dir);
    free(installed_bin_dir);
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

    char* bin_dir = repman_path_join(old_installed_path, "bin");

    char* local_path = repman_get_local_path();

    // check if new_installed_path is a dir already
    if (repman_dir_exists(new_installed_path)) {
        fprintf(stderr, "Package already exists: %s\n", new_installed_path);
        rc = -1;
        goto cleanup;
    }

    repman_rm(download_dir);
    if (repman_mkdir_p(download_dir) != 0) {
        fprintf(stderr, "Failed to create tmp directory: %s\n", download_dir);
        rc = -1;
        goto cleanup;
    }

    char* pkg_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz");
    char* sig_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.minisig");
    char* sha256_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.sha256");
    char* pkg_path = repman_path_join(download_dir, pkg_name);
    char* sig_path = repman_path_join(download_dir, sig_name);
    char* sha256_path = repman_path_join(download_dir, sha256_name);
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
    
    if (symlink_package_executables(new_installed_path, local_path) != 0) {
        rc = -1;
        goto cleanup;
    }

    printf("Package installed successfully.\n");
    

cleanup:
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

    char* pkg_and_ver = "test-v1.2.10";
    char* os = "ubuntu";
    char* arch = "amd64";
    repman_download_and_install_pkg(PACKAGE_URL, pkg_and_ver, os, arch);

    return 0;
}

// https://github.com/Polarstingray/packages/releases/download/test-v1.1.10/test_v1.2.10_ubuntu_amd64.tar.gz
// 