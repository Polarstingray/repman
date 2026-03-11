

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
    Verify the signature of a file using minisign.
    The minisig_path should point to the .minisig signature file corresponding to the file being verified.
    Returns 0 on success, -1 on failure.
*/
int repman_verify_minisig(const char *filepath, const char *minisig_path);


#endif