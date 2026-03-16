#ifndef REPMAN_INSTALL_H
#define REPMAN_INSTALL_H

#include <cjson/cJSON.h>

/* Functions from install.c */

int check_for_executables(const char *path);
char *repman_resolve_download(const char *url, const char *pkg_and_ver, const char *os, const char *arch, const char *ext);
char* repman_pkg_name(const char *pkg_and_ver, const char *os, const char *arch, const char *ext);
int repman_extract_tarball(const char *tarball_path, const char *dest_dir);
int repman_download_and_install_pkg(const char *url, const char *pkg_and_ver, const char *os, const char *arch);
int repman_install_latest(const char* name, const char* os, const char* arch);
#endif