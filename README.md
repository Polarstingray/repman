# repman

A minimal package manager built in C with a Python CLI. Packages are distributed as signed tarballs hosted on GitHub Releases. Every installation is verified with both SHA256 and a [minisign](https://jedisct1.github.io/minisign/) signature before anything is extracted to disk.

---

## How it works

repman is split into two layers:

**C library (`librepman.so`)** — all I/O-heavy work: downloading with libcurl, SHA256 and minisign verification (via `sha256sum`/`minisign` subprocesses), tarball extraction, symlink management, and JSON parsing with cJSON.

**Python CLI (`repcli.py`)** — a thin `argparse` frontend that loads `config.env`, calls into `librepman.so` via `ctypes`, and provides the user-facing subcommands.

### Package lifecycle

1. `repman update` — downloads `index.json` from the configured URL, verifies its SHA256 checksum and minisign signature against `ci.pub`, then atomically renames it into place.
2. `repman install <name>` — looks up the package in `index.json`, resolves the highest available version for the configured OS/arch, downloads the `.tar.gz` plus its `.sha256` and `.minisig` files, verifies both, extracts the tarball, and symlinks the package's `bin/`, `lib/`, and `data/` contents into `~/.local/`.
3. `repman upgrade` — iterates `installed.json`, checks each package against the index, and installs any that are behind.
4. `repman uninstall <name>` — removes the symlinks and the extracted package directory, and deletes the entry from `installed.json`.

### Security model

Every artifact that touches disk is verified before use:

| Artifact | SHA256 | minisign |
|---|---|---|
| `index.json` | `index.json.sha256` | `index.json.minisig` |
| Package tarball | `<pkg>.tar.gz.sha256` | `<pkg>.tar.gz.minisig` |

Verification fails hard — if either check fails, the file is not moved into place and installation is aborted. The index and packages are written atomically via `rename()` so a partial download or crash never leaves a corrupt file.

The public key (`ci.pub`) is the root of trust. Keep it and rotate it carefully.

---

## Directory layout

After install, repman uses the following directories under `$XDG_DATA_HOME/repman` (defaults to `~/.local/share/repman`):

```
~/.local/share/repman/
├── index/
│   ├── index.json          # current package index
│   └── installed.json      # { "name": "version", ... }
├── sig/
│   ├── ci.pub              # minisign public key
│   └── index/
│       ├── index.json.minisig
│       └── index.json.sha256
├── packages/
│   └── <name>_v<version>/  # extracted tarball per package
│       ├── bin/
│       ├── lib/            # optional
│       └── data/           # optional
├── cli/
│   ├── repman.py           # ctypes bindings
│   ├── repcli.py           # CLI entry point
│   └── venv/               # Python venv (python-dotenv)
├── build/
│   └── librepman.so
├── config.env              # local overrides (see Configuration)
├── tmp/                    # staging area, cleared after each operation
└── cache/
```

Package binaries and libraries are symlinked into `~/.local/bin/` and `~/.local/lib/`. Package data directories are symlinked into `~/.local/share/<name>`.

### `index.json` format

```json
{
  "package-name": {
    "latest": "1.2.0",
    "versions": {
      "1.2.0": {
        "targets": {
          "ubuntu_amd64": {
            "url": "https://github.com/owner/packages/releases/tag/package-v1.2.0"
          }
        }
      }
    }
  }
}
```

The `url` field points to the GitHub release *tag* page. repman rewrites it to the download URL internally and constructs filenames in the form `<name>_v<version>_<os>_<arch>.tar.gz`.

---

## Dependencies

### Build-time
- `gcc` (C11)
- `libcurl` dev headers — `sudo apt install libcurl4-openssl-dev`
- `libcjson` dev headers — `sudo apt install libcjson-dev`

### Runtime
- `minisign` — `sudo apt install minisign`
- `sha256sum` — part of `coreutils` (present by default)
- `python3` (3.8+)
- `python-dotenv` — installed automatically by `make install`

---

## Installation

### 1. Build the shared library

```sh
make
```

This produces `build/librepman.so`.

### 2. Install

```sh
make install
```

This does the following:

- Creates `~/.local/bin`, `~/.local/share/repman`, and all required subdirectories.
- Copies `build/` and `cli/` to `~/.local/share/repman/`.
- Copies `config.env.example` to `~/.local/share/repman/config.env` (skipped if already present, to preserve edits).
- Copies `sig/ci.pub` to `~/.local/share/repman/sig/ci.pub` if available locally; otherwise warns you to run `repman fetch-key`.
- Creates a Python venv at `~/.local/share/repman/cli/venv` and installs `python-dotenv`.
- Writes a `repman` shell wrapper to `~/.local/bin/repman`.

If `~/.local/bin` is not in your `PATH`, the installer prints the line to add to your shell profile.

### 3. First-time setup

If `ci.pub` was not present at install time, fetch the public key:

```sh
repman fetch-key
```

Then pull the package index:

```sh
repman update
```

You are now ready to install packages.

---

## Configuration

repman reads `~/.local/share/repman/config.env` at startup via `python-dotenv`. The values are injected into the process environment before any C library calls, so the C code reads them with `getenv()` (with compiled-in defaults as fallback).

| Variable | Default | Description |
|---|---|---|
| `OS` | `ubuntu` | Target OS name used for package resolution |
| `ARCH` | `amd64` | Target architecture used for package resolution |
| `PUBKEY_URL` | *(GitHub raw URL)* | Where to download the minisign public key |
| `INDEX_URL` | *(GitHub raw URL)* | Where to download `index.json` |
| `INDEX_SHA256_URL` | *(GitHub raw URL)* | Where to download `index.json.sha256` |
| `INDEX_MINISIG_URL` | *(GitHub raw URL)* | Where to download `index.json.minisig` |

See `config.env.example` for the full default values.

Environment variables already set in the calling shell take precedence over `config.env`. The test suite uses this to override `XDG_DATA_HOME` and URL endpoints per-test so no test touches real repman data.

---

## Usage

```
repman <command> [options]
```

### Commands

| Command | Description |
|---|---|
| `update` | Download, verify, and install the latest `index.json` |
| `install <name>` | Install the latest available version of a package |
| `install <name> -v <version>` | Install a specific version |
| `uninstall <name>` | Uninstall a package (removes symlinks and package directory) |
| `uninstall <name> -v <version>` | Uninstall a specific version |
| `upgrade` | Upgrade all installed packages that are behind the index |
| `list` | Print all installed packages and their versions |
| `fetch-key` | Download the minisign public key from `PUBKEY_URL` |
| `ensure-dirs` | Create all required data directories (idempotent) |

### Examples

```sh
# First-time setup
repman fetch-key
repman update

# Install a package
repman install ripgrep

# Install a specific version
repman install ripgrep -v 14.0.0

# Check what is installed
repman list

# Update the index and upgrade all packages
repman update && repman upgrade

# Remove a package
repman uninstall ripgrep
```

---

## Development

### Build and run C tests

```sh
make test
```

Compiles each `tests/*.c` file against the library sources and runs it.

### Run CLI integration tests

```sh
make test-cli
```

Runs `tests/test_cli.py` (20 tests) via `unittest`. Each test gets an isolated temp directory via `XDG_DATA_HOME` so nothing touches real repman data. Requires `librepman.so` to be built first.

### Clean

```sh
make clean
```

Removes the `build/` directory.

### Source layout

```
src/
├── util.c          # String helpers, file I/O, mkdir_p, rm_rf, curl download
├── verify.c        # SHA256 and minisign verification (fork+exec)
├── index.c         # Index parsing, version resolution, installed.json management
├── install.c       # Download, verify, extract, symlink, uninstall
└── lib/
    ├── util.h
    ├── verify.h
    ├── index.h
    └── install.h
cli/
├── repman.py       # ctypes bindings for librepman.so
└── repcli.py       # argparse CLI, config loading, command handlers
tests/
├── test_cli.py     # Integration tests (Python/unittest)
└── *.c             # Unit tests (C)
```

---

## Known limitations

The following features are not yet implemented:

- **`repman_update_key()`** — public key rotation is a stub.
- **`get-env` command** — registered in the parser but has no handler.
- **`verify` command** — registered in the parser but has no handler.
- **`config` command** — registered in the parser but has no handler.
- **Atomic public key download** — `fetch-key` writes the key directly without an atomic rename.
