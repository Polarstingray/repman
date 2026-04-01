#!/usr/bin/env python3

import os
import repman
import argparse

from dotenv import load_dotenv

DATA_DIR = repman.get_data_dir()
ENV_FILE = os.path.join(DATA_DIR, "config.env")
INDEX_PATH = os.path.join(DATA_DIR, "index", "index.json")
INSTALLED_PATH = os.path.join(DATA_DIR, "index", "installed.json")
PUBKEY_PATH = os.path.join(DATA_DIR, "sig", "index", "ci.pub")

load_dotenv(ENV_FILE)

OS = os.getenv("OS", "ubuntu")
ARCH = os.getenv("ARCH", "amd64")

PUBKEY_URL = os.getenv("PUBKEY_URL", "https://example.com/pubkey")
 
def update(_: argparse.Namespace) -> int:
    return repman.update_index()
     
def install(args: argparse.Namespace) -> int:
    if args.name == "repman":
        print("error: use 'repman upgrade' to update repman itself")
        return 1

    if (not args.version) :
        return repman.install_latest(args.name, OS, ARCH)

    # rm the old version from repman/packages/
    # full uninstall (rm symlinks + update installed.json) is not needed 
    return repman.install(args.name, args.version, OS, ARCH)

def uninstall(args: argparse.Namespace) -> int:
    if args.name == "repman":
        print("error: repman cannot be uninstalled")
        return 1
    if (not args.version) :
        return repman.uninstall(args.name)
    return repman.uninstall(args.name, args.version)

def upgrade(_: argparse.Namespace) -> int:
    return repman.upgrade(OS, ARCH)


def list_pkgs(_: argparse.Namespace) -> int:
    return repman.list_installed()


def fetch_public_key(_: argparse.Namespace) -> str:
    return repman.download(PUBKEY_URL, PUBKEY_PATH) # eventually implement atomic rewrite in C


def ensure_dirs(_: argparse.Namespace) -> None:
    return repman.ensure_dirs()

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Repman")
    sub = p.add_subparsers(dest="cmd", required=True)

    # update (fetch index)
    sp = sub.add_parser("update", help="Update the package index")
    # sp.add_argument("--index", default="index/index.json", help="Path to index.json (default: %(default)s)")
    sp.set_defaults(func=update)

    # upgrade (go through installed list, marked out of date, and install new pkgs)
    sp = sub.add_parser("upgrade", help="Install latest pkg for any that are out of date")
    sp.set_defaults(func=upgrade)

    # install 
    sp = sub.add_parser("install", help="installs latest version")
    sp.add_argument("name", help="Package name")
    sp.add_argument("-v", "--version", type=str, required=False, help="Install a specific version")
    sp.set_defaults(func=install)

    # list installed packages
    sp = sub.add_parser("list", help="List installed packages and their versions")
    sp.set_defaults(func=list_pkgs)

    # uninstall
    sp = sub.add_parser("uninstall", help="usage: uninstall <name> -> uninstalls a package")
    sp.add_argument("name", help="Package name")
    sp.add_argument("-v", "--version", type=str, required=False, help="Uninstall a specific version")
    sp.set_defaults(func=uninstall)

    sp = sub.add_parser("fetch-key", help="Fetch public key")
    sp.set_defaults(func=fetch_public_key)

    # get-env
    sp = sub.add_parser("get-env", help="Print key environment values")
    # sp.set_defaults(func=cmd_get_env)

    sp = sub.add_parser("ensure-dirs", help="Call once on first download")
    sp.set_defaults(func=ensure_dirs)

    # verify index
    sp = sub.add_parser("verify", help="verify the index.json")
    # sp.set_defaults(func=cmd_run)


    # config
    sp = sub.add_parser("config", help="Open config.env in $EDITOR (default nano)")
    # sp.set_defaults(func=cmd_config)
    return p


def main() :
    parser = build_parser()
    args = parser.parse_args()
    rc = args.func(args)  # type: ignore[attr-defined]
    raise SystemExit(rc)

main()
