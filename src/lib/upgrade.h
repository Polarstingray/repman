#ifndef REPMAN_UPGRADE_H
#define REPMAN_UPGRADE_H

#include "repman.h"

/*
    Iterate all installed packages, compare against the index, and install
    any that are behind. Calls repman_self_update for "repman" itself.
    Returns REPMAN_OK if all upgrades succeeded (or nothing to upgrade),
    REPMAN_ERR if any upgrade failed.
*/
int repman_upgrade(const char *os, const char *arch);

/*
    Print each installed package name and version to stdout.
    Returns REPMAN_OK on success, REPMAN_ERR on failure.
*/
int repman_list_installed(void);

/*
    Print each package in index.json whose latest version has a target for
    os_arch. Returns REPMAN_OK on success, REPMAN_ERR on failure.
*/
int repman_list_available(const char *os, const char *arch);

#endif /* REPMAN_UPGRADE_H */
