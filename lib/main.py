import os
import repman
import argparse

from dotenv import load_dotenv

DATA_DIR = repman.get_data_dir()
ENV_FILE = os.path.join(DATA_DIR, "config.env")
INDEX_PATH = os.path.join(DATA_DIR, "index", "index.json")
INSTALLED_PATH = os.path.join(DATA_DIR, "index", "installed.json")

OS = "ubuntu"
ARCH = "amd64"

load_dotenv(ENV_FILE)
 
def update(args: argparse.Namespace) -> int:
    return repman.update_index()
     
def install(args: argparse.Namespace) -> int:
    installed_version = repman.get_installed_version(INSTALLED_PATH, args.name)
    if (not installed_version) :
        installed_version = "0.0.0"
    
    version = repman.get_version(INDEX_PATH, args.name, installed_version, OS, ARCH)
    if (not version) :
        print(f"Failed to resolve version for {args.name}")
        return -1
    if (installed_version == version) :
        print(f"{args.name}_v{version} is up to date.")
        return 1
    
    print(f"{args.name}_v{version}")
    url = repman.get_pkg_url(INDEX_PATH, args.name, version, OS, ARCH)
    if (not url) :
        print(f"Failed to resolve download url for {args.name}")
    
    return install(url, args.name, version, OS, ARCH)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Repman")
    sub = p.add_subparsers(dest="cmd", required=True)

    # update (fetch index)
    sp = sub.add_parser("update", help="Update the package index")
    # sp.add_argument("--index", default="index/index.json", help="Path to index.json (default: %(default)s)")
    sp.set_defaults(func=update)

    # upgrade (go through installed list, marked out of date, and install new pkgs)
    sp = sub.add_parser("upgrade", help="Install latest pkg for any that are out of date")
    # sp.set_defaults(func=)

    # install 
    sp = sub.add_parser("install", help="installs latest version")
    sp.add_argument("name", help="Package name")
    sp.add_argument("-v", "--version", required=False, help="Install a specific version")
    sp.set_defaults(func=install)

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
