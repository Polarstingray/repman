#define _POSIX_C_SOURCE 200809L

#include "lib/selfupdate.h"
#include "lib/util.h"
#include "lib/index.h"
#include "lib/install.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int repman_self_update(const char *version, const char *os, const char *arch) {
    int rc = 0;

    size_t pav_len = strlen("repman-v") + strlen(version) + 1;
    char *pkg_and_ver = malloc(pav_len);
    if (!pkg_and_ver) return -1;
    snprintf(pkg_and_ver, pav_len, "repman-v%s", version);

    char *index_path    = repman_full_path("index", "index.json");
    char *installed_json = repman_full_path("index", "installed.json");
    char *data_dir      = repman_get_data_dir();
    char *local_path    = repman_get_local_path();
    char *download_dir  = repman_path_join(data_dir, "tmp");
    char *staging_dir   = repman_path_join(download_dir, "repman_self_update");

    char *url = repman_get_pkg_url(index_path, "repman", version, os, arch);
    if (!url) {
        fprintf(stderr, "repman not found in index for version %s\n", version);
        rc = -1;
        goto cleanup_early;
    }

    char *pkg_url    = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz");
    char *sig_url    = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.minisig");
    char *sha256_url = repman_resolve_download(url, pkg_and_ver, os, arch, ".tar.gz.sha256");
    free(url);

    repman_rm(download_dir);
    if (repman_mkdir_p(download_dir) != 0) {
        fprintf(stderr, "Failed to create tmp directory\n");
        rc = -1;
        goto cleanup_urls;
    }

    char *pkg_name    = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz");
    char *sig_name    = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.minisig");
    char *sha256_name = repman_pkg_name(pkg_and_ver, os, arch, ".tar.gz.sha256");
    char *pkg_path    = repman_path_join(download_dir, pkg_name);
    char *sig_path    = repman_path_join(download_dir, sig_name);
    char *sha256_path = repman_path_join(download_dir, sha256_name);
    free(pkg_name); free(sig_name); free(sha256_name);

    if (!pkg_url || !sig_url || !sha256_url) {
        fprintf(stderr, "Failed to resolve self-update download URLs\n");
        rc = -1;
        goto cleanup_paths;
    }

    if (download_package_files(pkg_url, sig_url, sha256_url, pkg_path, sig_path, sha256_path) != 0) {
        rc = -1;
        goto cleanup_paths;
    }

    if (verify_package_files(pkg_path, sig_path, sha256_path) != 0) {
        rc = -1;
        goto cleanup_paths;
    }

    repman_rm(staging_dir);
    if (repman_mkdir_p(staging_dir) != 0) {
        fprintf(stderr, "Failed to create staging directory\n");
        rc = -1;
        goto cleanup_paths;
    }

    if (repman_extract_tarball(pkg_path, staging_dir) != 0) {
        fprintf(stderr, "Failed to extract self-update tarball\n");
        rc = -1;
        goto cleanup_staging;
    }

    char *extracted_root = repman_path_join(staging_dir, "repman");
    char *extracted_bin  = repman_path_join(extracted_root, "bin");

    if (check_extracted_package(extracted_root, extracted_bin) != 0) {
        rc = -1;
        free(extracted_bin); free(extracted_root);
        goto cleanup_staging;
    }
    free(extracted_bin);

    /* Atomic copy: read source, write to dest (repman_write_file uses .tmp+rename).
     * Source paths are relative to extracted_root and match the layout produced by setup.sh:
     *   data/build/librepman.so, data/cli/repcli.py, data/cli/repman.py
     */
    struct {
        const char *subpath;   /* relative to extracted_root */
        char *dest;            /* absolute destination path  */
    } copies[] = {
        { "data/build/librepman.so", repman_full_path("build", "librepman.so") },
        { "data/cli/repcli.py",      repman_full_path("cli",   "repcli.py")   },
        { "data/cli/repman.py",      repman_full_path("cli",   "repman.py")   },
    };
    int n_copies = (int)(sizeof(copies) / sizeof(copies[0]));

    for (int i = 0; i < n_copies && rc == 0; i++) {
        char *src = repman_path_join(extracted_root, copies[i].subpath);
        size_t len;
        char *buf = repman_read_file(src, &len);
        free(src);
        if (!buf) {
            fprintf(stderr, "Failed to read %s from self-update package\n", copies[i].subpath);
            rc = -1;
        } else {
            if (repman_write_file(copies[i].dest, buf, len) != 0) {
                fprintf(stderr, "Failed to write %s\n", copies[i].dest);
                rc = -1;
            }
            free(buf);
        }
    }

    /* Free dest paths */
    for (int i = 0; i < n_copies; i++) free(copies[i].dest);

    /* Regenerate the wrapper script at ~/.local/bin/repman with absolute paths,
     * matching the format written by make install. The tarball's bin/repman uses
     * SCRIPT_DIR-relative paths which break when installed to ~/.local/bin/. */
    if (rc == 0) {
        char *wrapper_path = repman_path_join(local_path, "bin/repman");
        char *venv_python  = repman_path_join(data_dir, "cli/venv/bin/python3");
        char *cli_path     = repman_path_join(data_dir, "cli/repcli.py");

        /* Build wrapper content: #!/bin/sh\nexec "<python3>" "<repcli.py>" "$@"\n */
        size_t wrapper_len = strlen("#!/bin/sh\nexec \"") + strlen(venv_python)
                           + strlen("\" \"") + strlen(cli_path)
                           + strlen("\" \"$@\"\n") + 1;
        char *wrapper = malloc(wrapper_len);
        if (!wrapper) {
            rc = -1;
        } else {
            snprintf(wrapper, wrapper_len,
                     "#!/bin/sh\nexec \"%s\" \"%s\" \"$@\"\n",
                     venv_python, cli_path);
            if (repman_write_file(wrapper_path, wrapper, strlen(wrapper)) != 0) {
                fprintf(stderr, "Failed to write repman wrapper script\n");
                rc = -1;
            } else {
                chmod(wrapper_path, 0755);
            }
            free(wrapper);
        }
        free(cli_path);
        free(venv_python);
        free(wrapper_path);
    }

    free(extracted_root);

    if (rc != 0) goto cleanup_staging;

    if (repman_update_installed(installed_json, "repman", version, "install") < 0) {
        fprintf(stderr, "Failed to update installed.json after self-update\n");
        rc = -1;
        goto cleanup_staging;
    }

    printf("repman updated to v%s successfully.\n", version);

cleanup_staging:
    repman_rm(staging_dir);
cleanup_paths:
    free(pkg_path); free(sig_path); free(sha256_path);
cleanup_urls:
    free(pkg_url); free(sig_url); free(sha256_url);
cleanup_early:
    free(staging_dir);
    free(download_dir);
    free(local_path);
    free(data_dir);
    free(installed_json);
    free(index_path);
    free(pkg_and_ver);
    return rc;
}
