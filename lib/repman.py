#!/usr/bin/env python3

import ctypes
import json
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

# cJSON type
cJSON = ctypes.c_void_p

# util.h functions
_lib.repman_str_dup.restype = ctypes.c_void_p
_lib.repman_str_dup.argtypes = [ctypes.c_char_p]
_lib.repman_path_join.restype = ctypes.c_void_p
_lib.repman_path_join.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_str_repl.restype = ctypes.c_void_p
_lib.repman_str_repl.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_str_ends_with.restype = ctypes.c_int
_lib.repman_str_ends_with.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_read_file.restype = ctypes.c_void_p
_lib.repman_read_file.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_size_t)]
_lib.repman_write_file.restype = ctypes.c_int
_lib.repman_write_file.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t]
_lib.repman_file_exists.restype = ctypes.c_int
_lib.repman_file_exists.argtypes = [ctypes.c_char_p]
_lib.repman_dir_exists.restype = ctypes.c_int
_lib.repman_dir_exists.argtypes = [ctypes.c_char_p]
_lib.repman_mkdir_p.restype = ctypes.c_int
_lib.repman_mkdir_p.argtypes = [ctypes.c_char_p]
_lib.repman_rm.restype = ctypes.c_int
_lib.repman_rm.argtypes = [ctypes.c_char_p]
_lib.repman_get_local_path.restype = ctypes.c_void_p
_lib.repman_get_local_path.argtypes = []
_lib.repman_download.restype = ctypes.c_int
_lib.repman_download.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

# index.h functions
_lib.repman_full_path.restype = ctypes.c_void_p
_lib.repman_full_path.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_parse_json.restype = cJSON
_lib.repman_parse_json.argtypes = [ctypes.c_char_p]
_lib.cmp_versions.restype = ctypes.c_int
_lib.cmp_versions.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.get_version.restype = ctypes.c_void_p
_lib.get_version.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.get_pkg.restype = cJSON
_lib.get_pkg.argtypes = [cJSON, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_get_pkg_url.restype = ctypes.c_void_p
_lib.repman_get_pkg_url.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_update_installed.restype = ctypes.c_int
_lib.repman_update_installed.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_get_installed_version.restype = ctypes.c_void_p
_lib.repman_get_installed_version.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_is_pkg_behind.restype = ctypes.c_int
_lib.repman_is_pkg_behind.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

# install.h functions
_lib.check_for_executables.restype = ctypes.c_int
_lib.check_for_executables.argtypes = [ctypes.c_char_p]
_lib.repman_resolve_download.restype = ctypes.c_void_p
_lib.repman_resolve_download.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_pkg_name.restype = ctypes.c_void_p
_lib.repman_pkg_name.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_extract_tarball.restype = ctypes.c_int
_lib.repman_extract_tarball.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_install_latest.restype = ctypes.c_int
_lib.repman_install_latest.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

# verify.h functions
_lib.repman_verify_sha256.restype = ctypes.c_int
_lib.repman_verify_sha256.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.parse_sha256.restype = ctypes.c_int
_lib.parse_sha256.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
_lib.repman_verify_minisig.restype = ctypes.c_int
_lib.repman_verify_minisig.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

# load libc for free()
_libc = ctypes.CDLL(None)
_libc.free.argtypes = [ctypes.c_void_p]

# cJSON helpers
_lib.cJSON_Print.restype = ctypes.c_void_p
_lib.cJSON_Print.argtypes = [ctypes.c_void_p]
_lib.cJSON_Delete.restype = None
_lib.cJSON_Delete.argtypes = [ctypes.c_void_p]

def get_data_dir():
    ptr = _lib.repman_get_data_dir()
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

# util functions
def str_dup(src):
    ptr = _lib.repman_str_dup(src.encode())
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def path_join(base, name):
    ptr = _lib.repman_path_join(base.encode(), name.encode())
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def str_repl(s, s1, s2):
    ptr = _lib.repman_str_repl(s.encode(), s1.encode(), s2.encode())
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def str_ends_with(s, suffix):
    return _lib.repman_str_ends_with(s.encode(), suffix.encode())

def read_file(path):
    len_ptr = ctypes.c_size_t()
    ptr = _lib.repman_read_file(path.encode(), ctypes.byref(len_ptr))
    if not ptr:
        return None
    result = ctypes.string_at(ptr, len_ptr.value).decode()
    _libc.free(ptr)
    return result

def write_file(path, data):
    data_bytes = data.encode()
    return _lib.repman_write_file(path.encode(), data_bytes, len(data_bytes))

def file_exists(path):
    return _lib.repman_file_exists(path.encode())

def dir_exists(path):
    return _lib.repman_dir_exists(path.encode())

def mkdir_p(path):
    return _lib.repman_mkdir_p(path.encode())

def rm(path):
    return _lib.repman_rm(path.encode())

def get_local_path():
    ptr = _lib.repman_get_local_path()
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def download(url, dest_path):
    return _lib.repman_download(url.encode(), dest_path.encode())

# index functions
def full_path(dir_, name):
    ptr = _lib.repman_full_path(dir_.encode(), name.encode())
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def parse_json(filepath):
    ptr = _lib.repman_parse_json(filepath.encode())
    if not ptr:
        return None
    json_str_ptr = _lib.cJSON_Print(ptr)
    if not json_str_ptr:
        _lib.cJSON_Delete(ptr)
        return None
    json_str = ctypes.string_at(json_str_ptr).decode()
    _libc.free(json_str_ptr)
    _lib.cJSON_Delete(ptr)
    return json.loads(json_str)

def cmp_versions(a, b):
    return _lib.cmp_versions(a.encode(), b.encode())

def get_version(index_path, name, version, os, arch):
    ptr = _lib.get_version(index_path.encode(), name.encode(), version.encode(), os.encode(), arch.encode())
    if not ptr:
        return None
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def get_pkg_url(index_path, name, version, os, arch):
    ptr = _lib.repman_get_pkg_url(index_path.encode(), name.encode(), version.encode(), os.encode(), arch.encode())
    if not ptr:
        return None
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def update_installed(installed_path, name, version, option):
    return _lib.repman_update_installed(installed_path.encode(), name.encode(), version.encode(), option.encode())

def get_installed_version(filepath, name):
    ptr = _lib.repman_get_installed_version(filepath.encode(), name.encode())
    if not ptr:
        return None
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def is_pkg_behind(installed_path, index_path, name, os, arch):
    return _lib.repman_is_pkg_behind(installed_path.encode(), index_path.encode(), name.encode(), os.encode(), arch.encode())

# install functions
def check_for_executables(path):
    return _lib.check_for_executables(path.encode())

def resolve_download(url, pkg_and_ver, os, arch, ext):
    ptr = _lib.repman_resolve_download(url.encode(), pkg_and_ver.encode(), os.encode(), arch.encode(), ext.encode())
    if not ptr:
        return None
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def pkg_name(pkg_and_ver, os, arch, ext):
    ptr = _lib.repman_pkg_name(pkg_and_ver.encode(), os.encode(), arch.encode(), ext.encode())
    if not ptr:
        return None
    result = ctypes.string_at(ptr).decode()
    _libc.free(ptr)
    return result

def extract_tarball(tarball_path, dest_dir):
    return _lib.repman_extract_tarball(tarball_path.encode(), dest_dir.encode())

def install_latest(name, os, arch):
    return _lib.repman_install_latest(name.encode(), os.encode(), arch.encode())

# verify functions
def verify_sha256(filepath, sha256_path):
    return _lib.repman_verify_sha256(filepath.encode(), sha256_path.encode())

def parse_sha256(sha256_str):
    sha256 = (ctypes.c_char * 65)()
    ret = _lib.parse_sha256(sha256_str.encode(), sha256)
    if ret == 0:
        return sha256.value.decode()
    else:
        return None

def verify_minisig(filepath, minisig_path, pubkey_path):
    return _lib.repman_verify_minisig(filepath.encode(), minisig_path.encode(), pubkey_path.encode())





