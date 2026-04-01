#ifndef REPMAN_H
#define REPMAN_H

/* ── Shared constants ────────────────────────────────────────────────────── */
#define REPMAN_PATH_MAX  4096
#define REPMAN_NAME_MAX  32
#define REPMAN_OS_MAX    256

/* ── Canonical return codes ──────────────────────────────────────────────── */
#define REPMAN_OK         0   /* success                           */
#define REPMAN_ERR       -1   /* generic failure                   */
#define REPMAN_NOT_FOUND -2   /* resource not found / not installed */
#define REPMAN_ALREADY    1   /* already installed / up-to-date    */

#endif /* REPMAN_H */
