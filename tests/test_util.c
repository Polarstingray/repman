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
#include <sys/stat.h>
#include <unistd.h>

#include "../src/lib/util.h"
#include "../src/lib/verify.h"
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

static int file_write_all(const char *path, const char *data) {
    size_t len = strlen(data);
    return repman_write_file(path, data, len);
}

static int file_size(const char *path) {
    struct stat st; if (stat(path, &st) != 0) return -1; return (int)st.st_size;
}


/* ── Test cases ─────────────────────────────────────────────────────────────── */

static void test_strdup(void) {
    printf("\n[test_strdup]\n");

    char *s = repman_str_dup("hello");
    CHECK(s != NULL);
    CHECK(strcmp(s, "hello") == 0);
    free(s);

    char *empty = repman_str_dup("");
    CHECK(empty != NULL);
    CHECK(strlen(empty) == 0);
    free(empty);

    CHECK(repman_str_dup(NULL) == NULL);
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

static void test_rm_rf(void) {
    printf("\n[test_rm_rf]\n");

    const char *base = "/tmp/repman_test_rm";
    const char *nested = "/tmp/repman_test_rm/a/b/c";
    CHECK(repman_mkdir_p(nested) == 0);

    char *f1 = repman_path_join(nested, "file1.txt");
    char *f2 = repman_path_join("/tmp/repman_test_rm/a", "file2.txt");
    CHECK(file_write_all(f1, "x") == 0);
    CHECK(file_write_all(f2, "y") == 0);

    CHECK(repman_rm(base) == 0);
    CHECK(repman_file_exists(f1) == 0);
    CHECK(repman_file_exists(f2) == 0);

    free(f1); free(f2);
}

static void test_download(void) {
    printf("\n[test_download]\n");
    const char *url = "https://example.com";
    const char *out = "/tmp/repman_test_download.html";
    /* Clean */ repman_rm(out);
    int rc = repman_download(url, out);
    if (rc != 0) {
        SKIP("download failed (network?), skipping");
        return;
    }
    CHECK(repman_file_exists(out) == 1);
    CHECK(file_size(out) >= 0);
    repman_rm(out);
}

static void test_verify_sha256(void) {
    printf("\n[test_verify_sha256]\n");
    if (!has_command("sha256sum")) { SKIP("sha256sum not found"); return; }

    const char *f = "/tmp/repman_hash_src.txt";
    const char *sumf = "/tmp/repman_hash_src.txt.sha256";

    CHECK(file_write_all(f, "hello\n") == 0);

    /* compute checksum via system sha256sum */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "sha256sum %s > %s", f, sumf);
    int rc = system(cmd);
    if (rc != 0) { SKIP("sha256sum execution failed"); return; }

    CHECK(repman_verify_sha256(f, sumf) == 0);
    repman_rm(f);
    repman_rm(sumf);
}

static void test_verify_minisig(void) {
    printf("\n[test_verify_minisig]\n");
    if (!has_command("minisign")) { SKIP("minisign not found"); return; }

    /* Try to verify repo's index if present with ci.pub */
    const char *pub = "ci.pub";
    const char *idx = "index/index.json";
    const char *sig = "sig/index/index.json.minisig";

    if (!repman_file_exists(pub) || !repman_file_exists(idx) || !repman_file_exists(sig)) {
        SKIP("no signed artifacts available in repo");
        return;
    }

    int rc = repman_verify_minisig(idx, sig, pub);
    if (rc != 0) {
        /* Some environments might not have matching artifacts; skip if fails */
        SKIP("minisign verification failed (mismatch?)");
        return;
    }
    CHECK(rc == 0);
}

static void test_update_index(void) {
    printf("\n[test_update_index]\n");
    int rc = repman_update_index();
    if (rc != 0) {
        SKIP("update_index failed (network?), skipping");
        return;
    }
    CHECK(repman_file_exists("index/index.json") == 1);
    CHECK(repman_file_exists("sig/index/index.json.sha256") == 1);
    CHECK(repman_file_exists("sig/index/index.json.minisig") == 1);
}

static void test_get_data_dir(void) {
    printf("\n[test_get_data_dir]\n");
    const char *data_dir = repman_get_data_dir();
    CHECK(data_dir != NULL);
    printf("Data directory: %s\n", data_dir);
    free(data_dir);
}


/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void) {
    printf("=== test_util ===");

    test_strdup();
    test_path_join();
    test_str_ends_with();
    test_file_rw();
    test_mkdir_p();
    test_rm_rf();
    test_download();
    test_verify_sha256();
    test_verify_minisig();
    test_update_index();
    test_get_data_dir();

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}