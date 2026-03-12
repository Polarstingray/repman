

#include "lib/verify.h"
#include "lib/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PACKAGE_URL "https://github.com/Polarstingray/packages/releases/tag/test-v1.2.10"
#define OS "ubuntu"
#define ARCH "amd64"
#define PUBKEY "ci.pub"

// gcc src/verify.c src/util.c src/install.c src/index.c -lcurl -o ./build/install.o && ./build/install.o 
// && tree ~/.local/share/repman

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

int repman_download_pkg(const char* download_dir, const char *url, const char *pkg_and_ver, const char *os, const char *arch) {
    int rc = 0;
    char* pkg_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz");
    char* sig_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.minisig");
    char* sha256_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.sha256");

    char* pkg_path = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz");
    char* sig_path = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.minisig");
    char* sha256_path = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.sha256");

    if (!pkg_url || !sig_url || !sha256_url) {
        fprintf(stderr, "Failed to resolve download URLs\n");
        rc = 1;
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
    
cleanup:
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

    char* pkg_and_ver = "test-v1.2.10";
    char* os = "ubuntu";
    char* arch = "amd64";
    repman_download_pkg("", PACKAGE_URL, pkg_and_ver, os, arch);

    return 0;
}

// https://github.com/Polarstingray/packages/releases/download/test-v1.1.10/test_v1.2.10_ubuntu_amd64.tar.gz
// 