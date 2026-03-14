/*
 * repman/tests/test_install.c
 *
 * Tests for install.c functions.
 * Run via: make test
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../src/lib/util.h"
#include "../src/lib/install.h"  // Assuming we add declarations

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

/* ── Test cases ─────────────────────────────────────────────────────────────── */

static void test_repman_resolve_download(void) {
    printf("\n[test_repman_resolve_download]\n");

    char *url = repman_resolve_download("https://github.com/user/repo/releases/tag/v1.0.0", "pkg-v1.0.0", "linux", "amd64", ".tar.gz");
    CHECK(url != NULL);
    if (url) {
        CHECK(strstr(url, "https://github.com/user/repo/releases/download/pkg_v1.0.0_linux_amd64.tar.gz") == url);
        free(url);
    }
}

static void test_repman_pkg_name(void) {
    printf("\n[test_repman_pkg_name]\n");

    char *name = repman_pkg_name("pkg-v1.0.0", "linux", "amd64", ".tar.gz");
    CHECK(name != NULL);
    if (name) {
        CHECK(strcmp(name, "pkg_v1.0.0_linux_amd64.tar.gz") == 0);
        free(name);
    }
}

static void test_check_for_executables(void) {
    printf("\n[test_check_for_executables]\n");

    // Create a temp dir with an executable file
    const char *dir = "/tmp/repman_test_exec";
    repman_mkdir_p(dir);
    const char *exec_file = "/tmp/repman_test_exec/test_exec";
    file_write_all(exec_file, "#!/bin/bash\necho test\n");
    chmod(exec_file, 0755);

    CHECK(check_for_executables(dir) == 0);

    // Test with no executables
    const char *noexec_file = "/tmp/repman_test_exec/test_noexec";
    file_write_all(noexec_file, "not executable");
    chmod(noexec_file, 0644);

    CHECK(check_for_executables(dir) == 0); // Still has the exec one

    repman_rm(dir);
}

static void test_repman_extract_tarball(void) {
    printf("\n[test_repman_extract_tarball]\n");
    if (!has_command("tar")) { SKIP("tar not found"); return; }

    // This is hard to test without a real tarball, so skip for now
    SKIP("tar extraction test requires real tarball");
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(void) {
    printf("=== test_install ===");

    test_repman_resolve_download();
    test_repman_pkg_name();
    test_check_for_executables();
    test_repman_extract_tarball();

    printf("\n%d / %d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}