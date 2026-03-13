

#include "lib/verify.h"
#include "lib/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

#define PACKAGE_URL "https://github.com/Polarstingray/packages/releases/tag/test-v1.2.10"
#define OS "ubuntu"
#define ARCH "amd64"
#define PUBKEY "ci.pub"

// gcc src/verify.c src/util.c src/install.c src/index.c -lcurl -o ./build/install.o && ./build/install.o && tree ~/.local/share/repman

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
        perror("Failed to fork");
        return -1;
    }
    if (childpid == 0) {       /* child process */
        // tar -xzf <tarball> -C ~/.local/share/repman/packages/<pkg_name>_<version>
        printf("tar -xzf %s -C %s\n", tarball_path, dest_dir);
        execlp("tar", "tar", "-xzf", tarball_path, "-C", dest_dir, NULL);
        perror("Failed to execute tar");
        exit(EXIT_FAILURE);
    }
    /* parent process */
    int status;
    waitpid(childpid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "tar extract tarball failed with status %d\n", WEXITSTATUS(status));
        return -1;
    }

    fprintf(stdout, "Tarball extracted successfully.\n");
    return 0;
}

int repman_download_pkg(const char *url, const char *pkg_and_ver, const char *os, const char *arch) {
    int rc = 0;
    char* pkg_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz");
    char* sig_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.minisig");
    char* sha256_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.sha256");

    // construct full paths
    char* base_path = repman_get_data_dir();
    char* download_dir = repman_path_join(base_path, "tmp");

    char* pkg_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz");
    char* sig_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.minisig");
    char* sha256_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.sha256");
    char* pkg_path = repman_path_join(download_dir, pkg_name);
    char* sig_path = repman_path_join(download_dir, sig_name);
    char* sha256_path = repman_path_join(download_dir, sha256_name);
    free(pkg_name); free(sig_name); free(sha256_name);

    char* pkg_dir = repman_path_join(base_path, "packages");
    char* pkg_ver = repman_str_repl(repman_str_dup(pkg_and_ver), "-v", "_v");
    char* new_installed_path = repman_path_join(pkg_dir, pkg_ver);

    char *app_name = strtok(pkg_ver, "_v");
    printf("app_name: %s\n", app_name);
    char* old_installed_path = repman_path_join(pkg_dir, app_name);
    free(app_name);

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

    if (!pkg_url || !sig_url || !sha256_url) {
        fprintf(stderr, "Failed to resolve download URLs\n");
        rc = -1;
        goto cleanup;
    }

    if (repman_download(pkg_url, pkg_path) != 0) {
        fprintf(stderr, "Failed to download package\n");
        rc = -1;
        goto cleanup;
    } else if (repman_download(sig_url, sig_path) != 0) {
        fprintf(stderr, "Failed to download signature\n");
        rc = -1;
        goto cleanup;
    } else if (repman_download(sha256_url, sha256_path) != 0) {
        fprintf(stderr, "Failed to download SHA256\n");
        rc = -1;
        goto cleanup;
    }
    printf("Download complete.\n");

    repman_verify_minisig(pkg_path, sig_path, PUBKEY);
    repman_verify_sha256(pkg_path, sha256_path);
    printf("Verification Complete.\n");

    if (repman_extract_tarball(pkg_path, pkg_dir) != 0) {
        fprintf(stderr, "Failed to extract tarball\n");
        rc = -1;
        goto cleanup;
    }

    // sanity verification of extracted content (bin dir w/ bin, etc.)
    // check for bin dir
    char* bin_dir = repman_path_join(old_installed_path, "bin");
    if (!repman_dir_exists(bin_dir)) {
        fprintf(stderr, "Bin directory not found: %s\n", bin_dir);
        rc = -1;
        repman_rm(old_installed_path);
        goto cleanup;
    }
    // check for bin executable
    if (check_for_executables(bin_dir) != 0) {
        fprintf(stderr, "Bin directory contains no executables\n");
        rc = -1;
        repman_rm(old_installed_path);
        goto cleanup;
    }

    printf("renaming %s -> %s\n", old_installed_path, new_installed_path);
    if (rename(old_installed_path, new_installed_path) != 0) {
        fprintf(stderr, "Failed to rename package\n");
        rc = -1;
        repman_rm(old_installed_path);
        goto cleanup;
    }

    printf("Package installed successfully.\n");

    // atomic symlink rewrite
    // create symlink i.g. ~/.local/share/repman/packages/test-v1.2.10/... -> ~/.local/share/test/...


    // remove old version directory
    

cleanup:
    repman_rm(download_dir);
    free(download_dir);
    free(base_path);
    free(pkg_url);
    free(sig_url);
    free(sha256_url);
    free(pkg_path);
    free(sig_path);
    free(sha256_path);
    free(old_installed_path);
    free(new_installed_path);
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
    repman_download_pkg(PACKAGE_URL, pkg_and_ver, os, arch);

    return 0;
}

// https://github.com/Polarstingray/packages/releases/download/test-v1.1.10/test_v1.2.10_ubuntu_amd64.tar.gz
// 