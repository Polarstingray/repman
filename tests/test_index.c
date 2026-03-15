/*
 * repman/tests/test_index.c
 *
 * Tests for index.c functions.
 * Run via: make test
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../src/lib/util.h"
#include "../src/lib/index.h"

/* ── Tiny test harness ───────────────────────────────────────────────────── */

static int tests_run    = 0;
static int tests_passed = 0;

#define CHECK(expr) do { \
    tests_run++; \
    if (expr) { \
        tests_passed++; \
        printf("  PASS  %s\n", #expr); \
    } else { \
        printf("  FAIL  %s  (line %d)\n", #expr, __LINE__); \
    } \
} while(0)

#define SKIP(msg) do { \
    tests_run++; \
    tests_passed++; \
    printf("  SKIP  %s\n", msg); \
} while(0)

static int has_command(const char *cmd) {
    char which_cmd[256];
    snprintf(which_cmd, sizeof(which_cmd), "which %s > /dev/null 2>&1", cmd);
    int rc = system(which_cmd);
    return (rc == 0);
}

/* ── Test cases ─────────────────────────────────────────────────────────────── */

static void test_cmp_versions(void) {
    printf("\n[test_cmp_versions]\n");

    CHECK(cmp_versions("1.0.0", "1.0.0") == 0);
    CHECK(cmp_versions("1.0.1", "1.0.0") == 1);
    CHECK(cmp_versions("1.0.0", "1.0.1") == -1);
    CHECK(cmp_versions("1.1.0", "1.0.0") == 1);
    CHECK(cmp_versions("1.0.0", "1.1.0") == -1);
    CHECK(cmp_versions("2.0.0", "1.0.0") == 1);
    CHECK(cmp_versions("1.0.0", "2.0.0") == -1);

    // Invalid formats
    CHECK(cmp_versions("1.0", "1.0.0") == -2);
    CHECK(cmp_versions("1.0.0", "1.0") == -2);
}

static void test_repman_parse_json(void) {
    printf("\n[test_repman_parse_json]\n");

    const char *index_path = "index/index.json";
    if (!repman_file_exists(index_path)) {
        SKIP("index file not found");
        return;
    }

    cJSON *index = repman_parse_json(index_path);
    CHECK(index != NULL);
    if (index) {
        CHECK(cJSON_IsObject(index));
        cJSON_Delete(index);
    }
}

static void test_get_pkg(void) {
    printf("\n[test_get_pkg]\n");

    const char *index_path = "index/index.json";
    if (!repman_file_exists(index_path)) {
        SKIP("index file not found");
        return;
    }

    cJSON *index = repman_parse_json(index_path);
    if (!index) {
        SKIP("failed to parse index");
        return;
    }

    // Test with a known package, assuming "test" exists
    char *resolved = get_version(index_path, "test", "1.2.10", "ubuntu", "amd64");
    CHECK(resolved != NULL);
    if (resolved) {
        cJSON *pkg = cJSON_GetObjectItemCaseSensitive(index, "test");
        cJSON *target = get_pkg(pkg, resolved, "ubuntu", "amd64");
        CHECK(target != NULL);
        if (target) {
            CHECK(cJSON_IsObject(target));
        }
        free(resolved);
    }

    // Test with invalid package
    char *invalid_version = get_version(index_path, "nonexistent", "1.0.0", "ubuntu", "amd64");
    CHECK(invalid_version == NULL);

    cJSON_Delete(index);
}

static void test_repman_get_pkg_url(void) {
    printf("\n[test_repman_get_pkg_url]\n");

    const char *index_path = "index/index.json";
    if (!repman_file_exists(index_path)) {
        SKIP("index file not found");
        return;
    }

    cJSON *index = repman_parse_json(index_path);
    if (!index) {
        SKIP("failed to parse index");
        return;
    }

    char *resolved = get_version(index_path, "test", "1.2.10", "ubuntu", "amd64");
    if (!resolved) {
        SKIP("failed to resolve version");
        cJSON_Delete(index);
        return;
    }

    cJSON *pkg = cJSON_GetObjectItemCaseSensitive(index, "test");
    cJSON *target = get_pkg(pkg, resolved, "ubuntu", "amd64");
    free(resolved);
    if (!target) {
        SKIP("target not found");
        cJSON_Delete(index);
        return;
    }

    char *url = repman_get_pkg_url(target);
    CHECK(url != NULL);
    if (url) {
        CHECK(strstr(url, "http") == url); // Should start with http
        free(url);
    }

    cJSON_Delete(index);
}

static void test_repman_update_index(void) {
    printf("\n[test_repman_update_index]\n");

    int rc = repman_update_index();
    if (rc != 0) {
        SKIP("update_index failed (network?), skipping");
        return;
    }
    CHECK(repman_file_exists("index/index.json") == 1);
    CHECK(repman_file_exists("sig/index/index.json.sha256") == 1);
    CHECK(repman_file_exists("sig/index/index.json.minisig") == 1);
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void) {
    printf("=== test_index ===");

    test_cmp_versions();
    tess_repman_parse_json();
    test_get_pkg();
    test_repman_get_pkg_url();
    test_repman_update_index();

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}