/*
 * repman/tests/test_index.c
 *
 * Tests for index.c functions.
 * Run via: make test
 */

#include "../src/lib/index.h"
#include "test_harness.h"

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

    /* Invalid / edge-case formats */
    CHECK(cmp_versions("1.0", "1.0.0") == -2);
    CHECK(cmp_versions("1.0.0", "1.0") == -2);
    CHECK(cmp_versions("", "1.0.0") == -2);
    CHECK(cmp_versions("1.0.0", "") == -2);
    CHECK(cmp_versions("a.b.c", "1.0.0") == -2);
    /* NULL inputs */
    CHECK(cmp_versions(NULL, "1.0.0") == -2);
    CHECK(cmp_versions("1.0.0", NULL) == -2);
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

    char *resolved = get_version(index_path, "test", "1.2.10", "ubuntu", "amd64");
    if (!resolved) {
        SKIP("failed to resolve version");
        return;
    }

    char *url = repman_get_pkg_url(index_path, "test", resolved, "ubuntu", "amd64");
    free(resolved);

    CHECK(url != NULL);
    if (url) {
        CHECK(strstr(url, "http") == url); /* URL should start with http */
        free(url);
    }
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
    test_repman_parse_json();
    test_get_pkg();
    test_repman_get_pkg_url();
    test_repman_update_index();

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
