#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$ROOT_DIR/application"
QMAKE_BIN="$(command -v qmake-qt5 2>/dev/null || command -v qmake 2>/dev/null || true)"

if [[ -z "$QMAKE_BIN" ]]; then
    echo "Error: qmake not found." >&2
    exit 1
fi

case "$(uname -s)" in
    Linux)
        SPEC="linux-g++"
        MAKE_BIN="$(command -v make)"
        ;;
    Darwin)
        SPEC="macx-g++"
        MAKE_BIN="$(command -v make)"
        ;;
    *)
        SPEC="win32-g++"
        MAKE_BIN="$(command -v mingw32-make 2>/dev/null || command -v mingw32-make.exe 2>/dev/null || command -v make 2>/dev/null || true)"
        ;;
esac

if [[ -z "${MAKE_BIN:-}" ]]; then
    echo "Error: make tool not found." >&2
    exit 1
fi

BUILD_JOBS="${DGLAN_MAKE_JOBS:-1}"

cd "$APP_DIR"
rm -f Makefile-Core .qmake.stash
"$QMAKE_BIN" Core.pro -r -spec "$SPEC" "CONFIG+=release"
"$MAKE_BIN" -f Makefile-Core -j"$BUILD_JOBS"
