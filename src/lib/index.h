


#ifndef REPMAN_INDEX_H
#define REPMAN_INDEX_H

#include <cjson/cJSON.h>

/*

*/

int repman_update_index();
char* repman_full_path(const char *dir_, const char *name);
cJSON *repman_parse_index(const char *filepath);
cJSON *get_pkg(cJSON *index, const char* name, const char* version, const char* os, const char* arch);
char* repman_get_pkg_url(cJSON *target);


#endif
