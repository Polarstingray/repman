#define _POSIX_C_SOURCE 200809L

#include "lib/upgrade.h"
#include "lib/util.h"
#include "lib/index.h"
#include "lib/install.h"
#include "lib/selfupdate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

int repman_upgrade(const char *os, const char *arch) {
    char *installed_path = repman_full_path("index", "installed.json");
    char *index_path     = repman_full_path("index", "index.json");

    cJSON *installed = repman_parse_json(installed_path);
    if (installed == NULL) {
        fprintf(stderr, "Failed to read installed.json — run 'repman ensure-dirs' first\n");
        free(installed_path);
        free(index_path);
        return REPMAN_ERR;
    }

    int rc = REPMAN_OK;
    int upgrades = 0;
    cJSON *entry = NULL;

    cJSON_ArrayForEach(entry, installed) {
        const char *name = entry->string;
        const char *current_ver = entry->valuestring;
        if (name == NULL || current_ver == NULL) continue;

        char *latest_ver = get_version(index_path, name, current_ver, os, arch);
        if (latest_ver == NULL) continue;

        if (cmp_versions(current_ver, latest_ver) < 0) {
            upgrades++;
            int upgrade_rc;
            if (strcmp(name, "repman") == 0) {
                upgrade_rc = repman_self_update(latest_ver, os, arch);
            } else {
                upgrade_rc = repman_install_latest(name, os, arch);
            }
            if (upgrade_rc != 0) {
                fprintf(stderr, "Failed to upgrade %s to %s\n", name, latest_ver);
                rc = REPMAN_ERR;
            }
        }

        free(latest_ver);
    }

    if (upgrades == 0) {
        printf("Everything up-to-date\n");
    }

    cJSON_Delete(installed);
    free(installed_path);
    free(index_path);
    return rc;
}

int repman_list_installed(void) {
    char *installed_path = repman_full_path("index", "installed.json");

    if (!repman_file_exists(installed_path)) {
        printf("No packages installed.\n");
        free(installed_path);
        return REPMAN_OK;
    }

    cJSON *installed = repman_parse_json(installed_path);
    free(installed_path);

    if (installed == NULL) {
        fprintf(stderr, "Failed to read installed.json\n");
        return REPMAN_ERR;
    }

    if (cJSON_GetArraySize(installed) == 0) {
        printf("No packages installed.\n");
        cJSON_Delete(installed);
        return REPMAN_OK;
    }

    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, installed) {
        if (entry->string && entry->valuestring) {
            printf("%s  %s\n", entry->string, entry->valuestring);
        }
    }

    cJSON_Delete(installed);
    return REPMAN_OK;
}
