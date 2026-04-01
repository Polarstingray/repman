#!/usr/bin/env python3

import ctypes
import os

_lib_path = os.path.join(os.path.dirname(__file__), "../build/librepman.so")
_lib = ctypes.CDLL(_lib_path)


# void repman_ensure_dirs(void)
_lib.repman_ensure_dirs.restype = None
_lib.repman_ensure_dirs.argtypes = []

# int repman_update_index(void)
_lib.repman_update_index.restype = ctypes.c_int
_lib.repman_update_index.argtypes = []

# int repman_download_and_install_pkg(const char*, const char*, const char*, const char*)
_lib.repman_download_and_install_pkg.restype = ctypes.c_int
_lib.repman_download_and_install_pkg.argtypes = [
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_char_p,
]

# int repman_uninstall(const char*, const char*)
_lib.repman_uninstall.restype = ctypes.c_int
_lib.repman_uninstall.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

# char *repman_get_data_dir(void)  — returns a heap pointer
_lib.repman_get_data_dir.restype = ctypes.c_void_p
_lib.repman_get_data_dir.argtypes = []

# int repman_download(const char*, const char*)
_lib.repman_download.restype = ctypes.c_int
_lib.repman_download.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

# int cmp_versions(const char*, const char*)
_lib.cmp_versions.restype = ctypes.c_int
_lib.cmp_versions.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

# char *get_version(const char*, const char*, const char*, const char*, const char*)
_lib.get_version.restype = ctypes.c_void_p
_lib.get_version.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

# int repman_install_latest(const char*, const char*, const char*)
_lib.repman_install_latest.restype = ctypes.c_int
_lib.repman_install_latest.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

# int repman_self_update(const char*, const char*, const char*)
_lib.repman_self_update.restype = ctypes.c_int
_lib.repman_self_update.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

# int repman_update_installed(const char*, const char*, const char*, const char*)
_lib.repman_update_installed.restype = ctypes.c_int
_lib.repman_update_installed.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

# int repman_upgrade(const char*, const char*)
_lib.repman_upgrade.restype = ctypes.c_int
_lib.repman_upgrade.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

# int repman_list_installed(void)
_lib.repman_list_installed.restype = ctypes.c_int
_lib.repman_list_installed.argtypes = []

# load libc for free()
_libc = ctypes.CDLL(None)
_libc.free.argtypes = [ctypes.c_void_p]


def get_data_dir():
    ptr = _lib.repman_get_data_dir()
    if not ptr:
        raise RuntimeError("repman_get_data_dir returned NULL")
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def install(name, version, os_name, arch) -> int:
    return _lib.repman_download_and_install_pkg(
        name.encode(),
        version.encode(),
        os_name.encode(),
        arch.encode(),
    )

def uninstall(name, version="") -> int:
    return _lib.repman_uninstall(name.encode(), version.encode())

def ensure_dirs():
    _lib.repman_ensure_dirs()

def update_index() -> int:
    return _lib.repman_update_index()

def download(url, dest_path):
    return _lib.repman_download(url.encode(), dest_path.encode())

def cmp_versions(a, b):
    return _lib.cmp_versions(a.encode(), b.encode())

def get_version(index_path, name, version, os, arch):
    ptr = _lib.get_version(index_path.encode(), name.encode(), version.encode(), os.encode(), arch.encode())
    if not ptr:
        return None
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def install_latest(name, os, arch):
    return _lib.repman_install_latest(name.encode(), os.encode(), arch.encode())

def self_update(version, os_name, arch) -> int:
    return _lib.repman_self_update(version.encode(), os_name.encode(), arch.encode())

def update_installed(installed_path, name, version, action="install") -> int:
    return _lib.repman_update_installed(
        installed_path.encode(), name.encode(), version.encode(), action.encode()
    )

def upgrade(os_name, arch) -> int:
    return _lib.repman_upgrade(os_name.encode(), arch.encode())

def list_installed() -> int:
    return _lib.repman_list_installed()
