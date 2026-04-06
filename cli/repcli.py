#!/usr/bin/env python3

import os
import subprocess
import repman
import argparse

from dotenv import load_dotenv

DATA_DIR = repman.get_data_dir()
ENV_FILE = os.path.join(DATA_DIR, "config.env")
INDEX_PATH = os.path.join(DATA_DIR, "index", "index.json")
INDEX_SHA256_PATH = os.path.join(DATA_DIR, "sig", "index", "index.json.sha256")
INDEX_MINISIG_PATH = os.path.join(DATA_DIR, "sig", "index", "index.json.minisig")
INSTALLED_PATH = os.path.join(DATA_DIR, "index", "installed.json")
PUBKEY_PATH = os.path.join(DATA_DIR, "sig", "index", "ci.pub")

load_dotenv(ENV_FILE)

OS = os.getenv("OS", "ubuntu")
ARCH = os.getenv("ARCH", "amd64")

PUBKEY_URL = os.getenv("PUBKEY_URL", "https://example.com/pubkey")
INDEX_URL = os.getenv("INDEX_URL", "")
INDEX_SHA256_URL = os.getenv("INDEX_SHA256_URL", "")
INDEX_MINISIG_URL = os.getenv("INDEX_MINISIG_URL", "")
 
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

def list_available_cmd(_: argparse.Namespace) -> int:
    return repman.list_available(OS, ARCH)


def fetch_public_key(_: argparse.Namespace) -> int:
    return repman.download_atomic(PUBKEY_URL, PUBKEY_PATH)


def cmd_get_env(_: argparse.Namespace) -> int:
    print(f"DATA_DIR:         {DATA_DIR}")
    print(f"OS:               {OS}")
    print(f"ARCH:             {ARCH}")
    print(f"PUBKEY_URL:       {PUBKEY_URL}")
    print(f"INDEX_URL:        {INDEX_URL}")
    print(f"INDEX_SHA256_URL: {INDEX_SHA256_URL}")
    print(f"INDEX_MINISIG_URL:{INDEX_MINISIG_URL}")
    return 0


def cmd_verify(_: argparse.Namespace) -> int:
    ok = True

    rc = repman.verify_sha256(INDEX_PATH, INDEX_SHA256_PATH)
    if rc == 0:
        print("sha256: OK")
    else:
        print("sha256: FAIL")
        ok = False

    rc = repman.verify_minisig(INDEX_PATH, INDEX_MINISIG_PATH, PUBKEY_PATH)
    if rc == 0:
        print("minisig: OK")
    else:
        print("minisig: FAIL")
        ok = False

    return 0 if ok else 1


def launch_tui(_: argparse.Namespace) -> int:
    try:
        from reptui import RepmanTUI
    except ImportError:
        print("error: 'textual' is not installed. Re-run 'make install' to update the venv.")
        return 1
    app = RepmanTUI(os_name=OS, arch=ARCH,
                    index_path=INDEX_PATH,
                    installed_path=INSTALLED_PATH)
    app.run()
    return 0


def cmd_config(_: argparse.Namespace) -> int:
    print(f"config: {ENV_FILE}")
    editor = os.getenv("EDITOR", "nano")
    return subprocess.call([editor, ENV_FILE])


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

    # list available packages
    sp = sub.add_parser("list-available", help="List all packages available for this OS/arch")
    sp.set_defaults(func=list_available_cmd)

    # uninstall
    sp = sub.add_parser("uninstall", help="usage: uninstall <name> -> uninstalls a package")
    sp.add_argument("name", help="Package name")
    sp.add_argument("-v", "--version", type=str, required=False, help="Uninstall a specific version")
    sp.set_defaults(func=uninstall)

    sp = sub.add_parser("fetch-key", help="Fetch public key")
    sp.set_defaults(func=fetch_public_key)

    # get-env
    sp = sub.add_parser("get-env", help="Print key environment values")
    sp.set_defaults(func=cmd_get_env)

    sp = sub.add_parser("ensure-dirs", help="Call once on first download")
    sp.set_defaults(func=ensure_dirs)

    # verify index
    sp = sub.add_parser("verify", help="Verify the local index.json against its sha256 and minisig")
    sp.set_defaults(func=cmd_verify)

    # config
    sp = sub.add_parser("config", help="Open config.env in $EDITOR (default nano)")
    sp.set_defaults(func=cmd_config)

    # tui
    sp = sub.add_parser("tui", help="Interactive terminal UI")
    sp.set_defaults(func=launch_tui)

    return p


def main() :
    parser = build_parser()
    args = parser.parse_args()
    rc = args.func(args)  # type: ignore[attr-defined]
    raise SystemExit(rc)

main()
