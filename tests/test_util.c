/*
 * repman/tests/test_util.c
 *
 * Simple self-contained tests for util.c.
 * No test framework needed — just assert() and print statements.
 * Run via:  make test
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../src/lib/util.h"

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


/* ── Test cases ─────────────────────────────────────────────────────────────── */

static void test_strdup(void) {
    printf("\n[test_strdup]\n");

    char *s = repman_strdup("hello");
    CHECK(s != NULL);
    CHECK(strcmp(s, "hello") == 0);
    free(s);

    char *empty = repman_strdup("");
    CHECK(empty != NULL);
    CHECK(strlen(empty) == 0);
    free(empty);

    CHECK(repman_strdup(NULL) == NULL);
}

static void test_path_join(void) {
    printf("\n[test_path_join]\n");

    char *p1 = repman_path_join("/var/lib/repman", "index.json");
    CHECK(p1 != NULL);
    CHECK(strcmp(p1, "/var/lib/repman/index.json") == 0);
    free(p1);

    /* base already has trailing slash */
    char *p2 = repman_path_join("/var/lib/repman/", "index.json");
    CHECK(p2 != NULL);
    CHECK(strcmp(p2, "/var/lib/repman/index.json") == 0);
    free(p2);

    CHECK(repman_path_join(NULL, "x") == NULL);
    CHECK(repman_path_join("x", NULL) == NULL);
}

static void test_str_ends_with(void) {
    printf("\n[test_str_ends_with]\n");

    CHECK(repman_str_ends_with("index.json", ".json") == 1);
    CHECK(repman_str_ends_with("index.json", ".tar.gz") == 0);
    CHECK(repman_str_ends_with("a", "a") == 1);
    CHECK(repman_str_ends_with("a", "ab") == 0);
    CHECK(repman_str_ends_with("", "") == 1);
    CHECK(repman_str_ends_with(NULL, ".json") == 0);
}

static void test_file_rw(void) {
    printf("\n[test_file_rw]\n");

    const char *path = "/tmp/repman_test_util.txt";
    const char *data = "hello repman\n";
    size_t      dlen = strlen(data);

    int rc = repman_write_file(path, data, dlen);
    CHECK(rc == 0);
    CHECK(repman_file_exists(path) == 1);

    size_t out_len = 0;
    char  *buf = repman_read_file(path, &out_len);
    CHECK(buf != NULL);
    CHECK(out_len == dlen);
    CHECK(strcmp(buf, data) == 0);
    free(buf);

    CHECK(repman_file_exists("/tmp/repman_no_such_file_xyz") == 0);
}

static void test_mkdir_p(void) {
    printf("\n[test_mkdir_p]\n");

    int rc = repman_mkdir_p("/tmp/repman_test_dir/a/b/c");
    CHECK(rc == 0);

    /* calling again should be idempotent (EEXIST is not an error) */
    rc = repman_mkdir_p("/tmp/repman_test_dir/a/b/c");
    CHECK(rc == 0);
}


/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void) {
    printf("=== test_util ===");

    test_strdup();
    test_path_join();
    test_str_ends_with();
    test_file_rw();
    test_mkdir_p();

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}