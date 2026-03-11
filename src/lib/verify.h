

#ifndef REPMAN_VERIFY_H
#define REPMAN_VERIFY_H


#include <stddef.h>  /* size_t */
#include "index.h"

/*
    Verify the SHA256 hash of a file against a provided hash file.
    The hash file should contain the expected SHA256 hash followed by the filename, similar to the output of `sha256sum`.
    Returns 0 on success, -1 on failure.
*/
int repman_verify_sha256(const char *filepath, const char *sha256_path);

/*
    Parse a SHA256 hash from a string.
    The input string is expected to be in the format produced by `sha256sum`, which is:
        <64-character SHA256 hash> <whitespace> <filename>
    returns 0 on success and fills the sha256 output buffer with the extracted hash.
    returns -1 on failure (e.g., if the input format is invalid or the hash
*/
int parse_sha256(char *sha256_str, char *sha256);


/*
    Verify the signature of a file using minisign.
    The minisig_path should point to the .minisig signature file corresponding to the file being verified.
    Returns 0 on success, -1 on failure.
*/
int repman_verify_minisig(const char *filepath, const char *minisig_path, const char *pubkey_path);


#endif