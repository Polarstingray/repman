#ifndef REPMAN_NET_H
#define REPMAN_NET_H

#include "repman.h"

/*
    Download the resource at `url` and write it to `dest_path`.
    Returns REPMAN_OK on success, REPMAN_ERR on failure.
*/
int repman_download(const char *url, const char *dest_path);

#endif /* REPMAN_NET_H */
