#ifndef REPMAN_SELFUPDATE_H
#define REPMAN_SELFUPDATE_H

#include "repman.h"

/*
    Download, verify, and atomically replace the repman binary and Python CLI
    with the given version.
    Returns REPMAN_OK on success, REPMAN_ERR on failure.
*/
int repman_self_update(const char *version, const char *os, const char *arch);

#endif /* REPMAN_SELFUPDATE_H */
