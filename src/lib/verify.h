

#ifndef REPMAN_VERIFY_H
#define REPMAN_VERIFY_H

#include <stddef.h>  /* size_t */
#include "repman.h"

/*
    Verify the SHA256 hash of a file against a provided hash file.
    Returns 0 on success, -1 on failure.
*/
int repman_verify_sha256(const char *filepath, const char *sha256_path);


/*
    Verify the signature of a file using minisign.
    Returns 0 on success, -1 on failure.
*/
int repman_verify_minisig(const char *filepath, const char *minisig_path, const char *pubkey_path);

#endif