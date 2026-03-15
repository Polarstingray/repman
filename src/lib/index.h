


#ifndef REPMAN_INDEX_H
#define REPMAN_INDEX_H

#include <cjson/cJSON.h>

/*

*/

int    repman_update_index(void);
char*  repman_full_path(const char *dir_, const char *name);
cJSON *repman_parse_json(const char *filepath);
int    cmp_versions(const char *a, const char *b);
char  *get_version(const char *index_path, const char* name, const char* version, const char* os, const char* arch);
cJSON *get_pkg(cJSON *pkg, const char* version, const char* os, const char* arch);
char*  repman_get_pkg_url(cJSON *target);

int    repman_update_installed(const char* installed_filepath, const char *name, const char *version, const char *option);
char*  repman_get_installed_version(const char * filepath, const char *name);
int    repman_is_pkg_behind(const char* isntalled_path, const char* index_path, const char *name, const char* os, const char* arch);

#endif
