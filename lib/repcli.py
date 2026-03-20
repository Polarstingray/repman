#!/usr/bin/env python3

import os
import repman
import argparse
import json

from dotenv import load_dotenv

DATA_DIR = repman.get_data_dir()
ENV_FILE = os.path.join(DATA_DIR, "config.env")
INDEX_PATH = os.path.join(DATA_DIR, "index", "index.json")
INSTALLED_PATH = os.path.join(DATA_DIR, "index", "installed.json")
PUBKEY_PATH = os.path.join(DATA_DIR, "sig", "CI.PUB")

PUBKEY_URL = os.getenv("PUBKEY_URL", "https://example.com/pubkey")

OS = "ubuntu"
ARCH = "amd64"

load_dotenv(ENV_FILE)
 
def update(args: argparse.Namespace) -> int:
    return repman.update_index()
     
def install(args: argparse.Namespace) -> int:
    
    if (not args.version) :
        return repman.install_latest(args.name, OS, ARCH)

    # rm the old version from repman/packages/
    # full uninstall (rm symlinks + update installed.json) is not needed 
    return repman.install(args.name, args.version, OS, ARCH)


def upgrade(args: argparse.Namespace) -> int:

    # get all installed packages
    try :
        with open(INSTALLED_PATH, "r") as f:
            installed = json.load(f)

        cnt = 0
        for pkg in installed:
            latest_ver = repman.get_version(INDEX_PATH, pkg, installed[pkg], OS, ARCH)
            if (latest_ver is not None) and (repman.cmp_versions(installed[pkg], latest_ver) < 0) :
                cnt += 1
                rc = repman.install_latest(pkg, OS, ARCH)
                if rc != 0 :
                    print(f"Failed to install {pkg} version {latest_ver} (latest: {installed[pkg]}")
                    return rc
    
    except Exception as e:
        print(f"Error reading {INSTALLED_PATH}: {e}")
        return 1
        
    if cnt == 0 :
        print("Everything up-to-date")
    return 0


def fetch_public_key(args: argparse.Namespace) -> str:
    return repman.download(PUBKEY_URL, PUBKEY_PATH) # eventually implement atomic rewrite in C

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

    sp = sub.add_parser("fetch-key", help="Fetch public key")
    sp.set_defaults(func=fetch_public_key)

    # get-env
    sp = sub.add_parser("get-env", help="Print key environment values")
    # sp.set_defaults(func=cmd_get_env)

    # uninstall
    sp = sub.add_parser("uninstall", help="usage: uninstall <pkg_name> -> uninstalls a package")
    sp.add_argument("name", help="Program name to uninstall")
    # sp.set_defaults(func=cmd_stage)

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
