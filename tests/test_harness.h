#ifndef REPMAN_TEST_HARNESS_H
#define REPMAN_TEST_HARNESS_H

/*
 * Shared test harness for repman C unit tests.
 * Include this header instead of duplicating boilerplate in each test file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../src/lib/util.h"

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
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int)st.st_size;
}

#endif /* REPMAN_TEST_HARNESS_H */
