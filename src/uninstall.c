#define _POSIX_C_SOURCE 200809L

#include "lib/uninstall.h"
#include "lib/util.h"
#include "lib/index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <cjson/cJSON.h>

static int repman_validate_pkg_name(const char *name) {
    if (name == NULL || name[0] == '\0') {
        fprintf(stderr, "Package name must not be empty\n");
        return REPMAN_ERR;
    }
    if (strstr(name, "..") != NULL || strchr(name, '/') != NULL) {
        fprintf(stderr, "Invalid package name: %s\n", name);
        return REPMAN_ERR;
    }
    return REPMAN_OK;
}

static int unlink_pkg_files(const char *name, const char *version, const char *type) {
    if (name == NULL || version == NULL || type == NULL) {
        fprintf(stderr, "Invalid arguments to unlink_pkg_files\n");
        return -1;
    }

    if (strcmp(type, "bin") != 0 && strcmp(type, "lib") != 0) {
        fprintf(stderr, "Unsupported type in unlink_pkg_files: %s\n", type);
        return -1;
    }

    char *data_dir = repman_get_data_dir();
    char *pkg_dir = repman_path_join(data_dir, "packages");
    size_t pkg_name_len = strlen(name) + strlen(version) + 4;
    char *pkg_dirname = malloc(pkg_name_len);
    if (!pkg_dirname) {
        free(data_dir);
        free(pkg_dir);
        return -1;
    }
    snprintf(pkg_dirname, pkg_name_len, "%s_v%s", name, version);

    char *pkg_path = repman_path_join(pkg_dir, pkg_dirname);
    char *pkg_type_path = repman_path_join(pkg_path, type);
    char *local_path = repman_get_local_path();
    char *local_type_path = repman_path_join(local_path, type);

    int rc = 0;
    if (repman_dir_exists(pkg_type_path) != 0) {
        // Nothing to remove if this package has no bin/lib directory
        rc = 0;
        goto cleanup;
    }

    DIR *dr = opendir(pkg_type_path);
    if (dr == NULL) {
        if (!strcmp(type, "lib")) {
            // lib may be optional; not an error when absent
            rc = 0;
        } else {
            perror("opendir");
            fprintf(stderr, "Failed to open package %s directory: %s\n", type, pkg_type_path);
            rc = -1;
        }
        goto cleanup;
    }

    struct dirent *de;
    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

        char *local_file = repman_path_join(local_type_path, de->d_name);
        if (local_file == NULL) {
            rc = -1;
            continue;
        }

        struct stat st;
        if (lstat(local_file, &st) != 0) {
            // file not present; continue to next
            free(local_file);
            continue;
        }

        if (!S_ISLNK(st.st_mode)) {
            free(local_file);
            continue;
        }

        if (unlink(local_file) != 0) {
            perror("unlink");
            fprintf(stderr, "Failed to unlink %s\n", local_file);
            rc = -1;
        }

        free(local_file);
    }

    closedir(dr);

cleanup:
    free(pkg_dirname);
    free(local_type_path);
    free(local_path);
    free(pkg_type_path);
    free(pkg_path);
    free(pkg_dir);
    free(data_dir);
    return rc;
}

static int unlink_pkg_data(const char *name) {
    if (name == NULL) return -1;

    char *local_path = repman_get_local_path();
    if (!local_path) return -1;

    char *share_dir = repman_path_join(local_path, "share");
    char *pkg_share = repman_path_join(share_dir, name);

    int rc = 0;
    struct stat st;
    if (lstat(pkg_share, &st) == 0) {
        if (S_ISLNK(st.st_mode) || S_ISREG(st.st_mode)) {
            if (unlink(pkg_share) != 0) {
                perror("unlink");
                fprintf(stderr, "Failed to unlink data path: %s", pkg_share);
                rc = -1;
            }
        } else if (S_ISDIR(st.st_mode)) {
            if (repman_rm(pkg_share) != 0) {
                fprintf(stderr, "Failed to remove data directory: %s", pkg_share);
                rc = -1;
            }
        }
    }

    free(pkg_share);
    free(share_dir);
    free(local_path);
    return rc;
}

int repman_uninstall(const char *name, const char *version) {
    if (repman_validate_pkg_name(name) != REPMAN_OK) return REPMAN_ERR;

    if (strcmp(name, "repman") == 0) {
        fprintf(stderr, "error: repman cannot be uninstalled\n");
        return -1;
    }

    char *installed_json = repman_full_path("index", "installed.json");
    char *curr_pkg_ver = NULL;
    if (version == NULL || strcmp(version, "") == 0) {
        curr_pkg_ver = repman_get_installed_version(installed_json, name);
        if (curr_pkg_ver == NULL) {
            fprintf(stderr, "Package not found: %s", name);
            free(installed_json);
            return -1;
        }
    } else {
        curr_pkg_ver = repman_str_dup(version);
    }

    int rc = 0;
    if (unlink_pkg_files(name, curr_pkg_ver, "bin") != 0) {
        rc = -1;
    }
    if (unlink_pkg_files(name, curr_pkg_ver, "lib") != 0) {
        rc = -1;
    }

    if (unlink_pkg_data(name) != 0) {
        rc = -1;
    }

    char *data_dir = repman_get_data_dir();
    char *packages_dir = repman_path_join(data_dir, "packages");

    size_t pkg_dirname_len = strlen(name) + strlen(curr_pkg_ver) + 4;
    char *pkg_dirname = malloc(pkg_dirname_len);
    if (!pkg_dirname) {
        fprintf(stderr, "Failed to allocate package directory name");
        rc = -1;
        goto after_pkg_rm;
    }

    snprintf(pkg_dirname, pkg_dirname_len, "%s_v%s", name, curr_pkg_ver);
    char *pkg_path = repman_path_join(packages_dir, pkg_dirname);

    if (repman_rm(pkg_path) != 0) {
        fprintf(stderr, "Failed to remove package directory: %s", pkg_path);
        rc = -1;
    }

    free(pkg_path);
    free(pkg_dirname);

after_pkg_rm:
    free(packages_dir);
    free(data_dir);

    cJSON *installed = repman_parse_json(installed_json);
    if (installed != NULL) {
        cJSON_DeleteItemFromObject(installed, name);
        char *installed_str = cJSON_Print(installed);
        if (installed_str != NULL) {
            if (repman_write_file(installed_json, installed_str, strlen(installed_str)) != 0) {
                fprintf(stderr, "Failed to update installed.json");
                rc = -1;
            }
            free(installed_str);
        } else {
            fprintf(stderr, "Failed to render installed.json");
            rc = -1;
        }
        cJSON_Delete(installed);
    } else {
        fprintf(stderr, "Failed to parse installed.json for uninstall");
        rc = -1;
    }

    free(curr_pkg_ver);
    free(installed_json);

    return rc;
}
