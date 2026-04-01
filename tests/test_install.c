/*
 * repman/tests/test_install.c
 *
 * Tests for install.c functions.
 * Run via: make test
 */

#include "../src/lib/install.h"
#include "../src/lib/uninstall.h"
#include "test_harness.h"

/* ── Test cases ─────────────────────────────────────────────────────────────── */

static void test_repman_resolve_download(void) {
    printf("\n[test_repman_resolve_download]\n");

    const char *expected = "https://github.com/user/repo/releases/download/v1.0.0/pkg_v1.0.0_linux_amd64.tar.gz";
    char *url = repman_resolve_download("https://github.com/user/repo/releases/tag/v1.0.0", "pkg-v1.0.0", "linux", "amd64", ".tar.gz");
    CHECK(url != NULL);
    if (url) {
        CHECK(strcmp(url, expected) == 0);
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
