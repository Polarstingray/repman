#!/bin/sh
# install.sh — XDG-aware installer bundled inside each repman package tarball.
# Place bin/, lib/, and share/ next to this script in the tarball.
# All packages must include a bin/ directory with at least one executable.
# lib/ and share/ are optional.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

BIN_DIR="${HOME}/.local/bin"
LIB_DIR="${HOME}/.local/lib"
DATA_DIR="${XDG_DATA_HOME:-${HOME}/.local/share}"

# bin/ is required
if [ ! -d "${SCRIPT_DIR}/bin" ]; then
    echo "error: no bin/ directory found — package is missing its executable" >&2
    exit 1
fi

mkdir -p "${BIN_DIR}"
for f in "${SCRIPT_DIR}/bin/"*; do
    [ -f "$f" ] || continue
    cp "$f" "${BIN_DIR}/"
    chmod +x "${BIN_DIR}/$(basename "$f")"
    echo "installed: ${BIN_DIR}/$(basename "$f")"
done

# lib/ is optional
if [ -d "${SCRIPT_DIR}/lib" ]; then
    mkdir -p "${LIB_DIR}"
    cp -r "${SCRIPT_DIR}/lib/." "${LIB_DIR}/"
    echo "installed: lib/ -> ${LIB_DIR}/"
fi

# share/ is optional — preserves subdirectory layout (e.g. share/<pkg_name>/)
if [ -d "${SCRIPT_DIR}/share" ]; then
    mkdir -p "${DATA_DIR}"
    cp -r "${SCRIPT_DIR}/share/." "${DATA_DIR}/"
    echo "installed: share/ -> ${DATA_DIR}/"
fi

# warn if bin dir is not on PATH
case ":${PATH}:" in
    *":${BIN_DIR}:"*) ;;
    *)
        echo ""
        echo "warning: ${BIN_DIR} is not in your PATH"
        echo "  add the following to your shell profile:"
        echo "    export PATH=\"\${HOME}/.local/bin:\${PATH}\""
        ;;
esac
