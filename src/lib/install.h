#ifndef REPMAN_INSTALL_H
#define REPMAN_INSTALL_H

#include <cjson/cJSON.h>
#include "repman.h"

/* Public install API */
int   check_for_executables(const char *path);
char *repman_resolve_download(const char *url, const char *pkg_and_ver, const char *os, const char *arch, const char *ext);
char *repman_pkg_name(const char *pkg_and_ver, const char *os, const char *arch, const char *ext);
int   repman_extract_tarball(const char *tarball_path, const char *dest_dir);
int   repman_download_and_install_pkg(const char *name, const char *version, const char *os, const char *arch);
int   repman_install_latest(const char *name, const char *os, const char *arch);

/* Shared internal helpers used by selfupdate.c */
int   download_package_files(const char *pkg_url, const char *sig_url, const char *sha256_url,
                              const char *pkg_path, const char *sig_path, const char *sha256_path);
int   verify_package_files(const char *pkg_path, const char *sig_path, const char *sha256_path);
int   check_extracted_package(const char *old_installed_path, const char *bin_dir);

#endif /* REPMAN_INSTALL_H */
