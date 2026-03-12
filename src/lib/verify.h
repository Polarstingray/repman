

#ifndef REPMAN_VERIFY_H
#define REPMAN_VERIFY_H

#include <stddef.h>  /* size_t */

/*
    Verify the SHA256 hash of a file against a provided hash file.
    Returns 0 on success, -1 on failure.
*/
int repman_verify_sha256(const char *filepath, const char *sha256_path);

/* helper used internally, kept visible for tests if needed */
int parse_sha256(char *sha256_str, char *sha256);

/*
    Verify the signature of a file using minisign.
    Returns 0 on success, -1 on failure.
*/
int repman_verify_minisig(const char *filepath, const char *minisig_path, const char *pubkey_path);

#endif