#ifndef REPMAN_UNINSTALL_H
#define REPMAN_UNINSTALL_H

#include "repman.h"

/*
    Remove a package's symlinks, data directory, and package directory,
    then update installed.json.
    Pass version=NULL or "" to use the currently installed version.
    Returns REPMAN_OK on success, REPMAN_ERR on failure.
*/
int repman_uninstall(const char *name, const char *version);

#endif /* REPMAN_UNINSTALL_H */
