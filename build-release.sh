#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

VERSION=""
SKIP_BUILD=0
SKIP_PUBLISH=0
VERSION_FILE="application/Common/Version.h"
VERSION_FILE_ORIGINAL_B64=""
RESTORE_VERSION_FILE=0

cleanup() {
    local exit_code=$?

    if [[ "$RESTORE_VERSION_FILE" -eq 1 && -n "$VERSION_FILE_ORIGINAL_B64" ]]; then
        printf '%s' "$VERSION_FILE_ORIGINAL_B64" | base64 --decode > "$VERSION_FILE"
    fi

    exit "$exit_code"
}

trap cleanup EXIT

usage() {
    cat <<'USAGE'
Usage: ./build-release.sh [-SkipBuild] [-SkipPublish] [-Version X.Y.Z]

Linux-native release builder for DG-LAN.
- Builds Core + GUI with system Qt5/protobuf
- Produces a tarball asset under dist/
- Keeps local Version.h clean when publishing is skipped
- Optionally tags and uploads the tarball to the matching GitHub release
USAGE
}

require_tool() {
    local tool="$1"

    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Error: required tool not found: $tool" >&2
        exit 1
    fi
}

set_version_define() {
    local define_name="$1"
    local define_value="$2"

    sed -i "s/#define ${define_name} \"[^\"]*\"/#define ${define_name} \"${define_value}\"/" "$VERSION_FILE"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -SkipBuild) SKIP_BUILD=1; shift ;;
        -SkipPublish) SKIP_PUBLISH=1; shift ;;
        -Version)
            VERSION="${2:-}"
            if [[ -z "$VERSION" ]]; then
                echo "Error: -Version requires a value" >&2
                exit 1
            fi
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage
            exit 1
            ;;
    esac
done

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "Error: build-release.sh is Linux-only. Use build-release.ps1 on Windows." >&2
    exit 1
fi

ARCH="$(uname -m)"
case "$ARCH" in
    x86_64) ARCH_TAG="linux-x86_64" ;;
    aarch64|arm64) ARCH_TAG="linux-arm64" ;;
    armv7l) ARCH_TAG="linux-armv7" ;;
    *) ARCH_TAG="linux-$ARCH" ;;
esac

QMAKE_BIN="$(command -v qmake-qt5 2>/dev/null || command -v qmake 2>/dev/null || true)"
if [[ -z "$QMAKE_BIN" ]]; then
    echo "Error: qmake not found. Install Qt5 development packages first." >&2
    exit 1
fi

QT_BIN_DIR="$("$QMAKE_BIN" -query QT_HOST_BINS 2>/dev/null || true)"
if [[ -z "$QT_BIN_DIR" ]]; then
    QT_BIN_DIR="$("$QMAKE_BIN" -query QT_INSTALL_BINS 2>/dev/null || true)"
fi

LRELEASE_BIN="$(command -v lrelease-qt5 2>/dev/null || command -v lrelease 2>/dev/null || true)"
if [[ -z "$LRELEASE_BIN" && -n "$QT_BIN_DIR" ]]; then
    if [[ -x "$QT_BIN_DIR/lrelease-qt5" ]]; then
        LRELEASE_BIN="$QT_BIN_DIR/lrelease-qt5"
    elif [[ -x "$QT_BIN_DIR/lrelease" ]]; then
        LRELEASE_BIN="$QT_BIN_DIR/lrelease"
    fi
fi

for tool in protoc make gcc git tar base64; do
    require_tool "$tool"
done

if [[ ! -f "$VERSION_FILE" ]]; then
    echo "Error: missing $VERSION_FILE" >&2
    exit 1
fi

CURRENT_VERSION="$(sed -n 's/^#define VERSION "\(.*\)"/\1/p' "$VERSION_FILE")"
VERSION_TAG="$(sed -n 's/^#define VERSION_TAG "\(.*\)"/\1/p' "$VERSION_FILE")"
BUILD_TIME="$(date -u '+%Y-%m-%d_%H-%M')"
GIT_HASH="$(git rev-parse HEAD | cut -c1-12)"

if [[ -z "$CURRENT_VERSION" ]]; then
    echo "Error: unable to read VERSION from $VERSION_FILE" >&2
    exit 1
fi

if [[ -z "$VERSION" ]]; then
    if [[ "$SKIP_BUILD" -eq 0 ]]; then
        if [[ "$CURRENT_VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
            VERSION="${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.$((BASH_REMATCH[3] + 1))"
            echo "Version auto-incremented to: $VERSION"
        else
            echo "Error: VERSION is not in X.Y.Z form: $CURRENT_VERSION" >&2
            exit 1
        fi
    else
        VERSION="$CURRENT_VERSION"
        echo "Using existing version: $VERSION"
    fi
else
    echo "Version set to: $VERSION"
fi

if [[ "$SKIP_BUILD" -eq 0 ]]; then
    VERSION_FILE_ORIGINAL_B64="$(base64 -w0 "$VERSION_FILE")"
    RESTORE_VERSION_FILE=1

    set_version_define VERSION "$VERSION"
    set_version_define BUILD_TIME "$BUILD_TIME"
    set_version_define GIT_VERSION "$GIT_HASH"
    echo "Build time: $BUILD_TIME  Git: $GIT_HASH"
fi

CORE_BIN="application/Core/output/release/DG-LAN.Core"
GUI_BIN="application/GUI/output/release/DG-LAN.GUI"

if [[ "$SKIP_BUILD" -eq 0 ]]; then
    echo
    echo "=== Building Linux release (${ARCH_TAG}) ==="
    echo "Using qmake: $QMAKE_BIN"
    echo "Note: forcing top-level make -j1 because recursive qmake builds race on Linux."

    pushd application >/dev/null
    find . -name '.qmake.stash' -delete
    rm -f Makefile-Core Makefile-GUI
    rm -f Core/output/release/DG-LAN.Core GUI/output/release/DG-LAN.GUI

    "$QMAKE_BIN" GUI.pro -r -spec linux-g++ "CONFIG+=release"
    make -f Makefile-GUI -j1

    "$QMAKE_BIN" Core.pro -r -spec linux-g++ "CONFIG+=release"
    make -f Makefile-Core -j1
    popd >/dev/null
fi

for output in "$CORE_BIN" "$GUI_BIN"; do
    if [[ ! -f "$output" ]]; then
        echo "Error: build output missing: $output" >&2
        exit 1
    fi
done

PACKAGE_VERSION_SUFFIX=""
if [[ -n "$VERSION_TAG" ]]; then
    PACKAGE_VERSION_SUFFIX="-$VERSION_TAG"
fi

DIST_DIR="dist"
PACKAGE_NAME="DG-LAN-${VERSION}${PACKAGE_VERSION_SUFFIX}-${ARCH_TAG}"
STAGE_DIR="$DIST_DIR/$PACKAGE_NAME"
TARBALL_PATH="$DIST_DIR/$PACKAGE_NAME.tar.gz"
CHECKSUM_PATH="$TARBALL_PATH.sha256"

rm -rf "$STAGE_DIR" "$TARBALL_PATH" "$CHECKSUM_PATH"
mkdir -p "$STAGE_DIR/languages"

cp "$CORE_BIN" "$STAGE_DIR/"
cp "$GUI_BIN" "$STAGE_DIR/"
cp README.md BUILD.md COPYING "$STAGE_DIR/"

if [[ -d application/styles ]]; then
    cp -R application/styles "$STAGE_DIR/"
fi

LANGUAGE_COUNT=0
if [[ -n "$LRELEASE_BIN" && -d application/translations ]]; then
    while IFS= read -r -d '' ts_file; do
        qm_name="$(basename "${ts_file%.ts}.qm")"
        "$LRELEASE_BIN" "$ts_file" -qm "$STAGE_DIR/languages/$qm_name" >/dev/null
        LANGUAGE_COUNT=$((LANGUAGE_COUNT + 1))
    done < <(find application/translations -maxdepth 1 -name '*.ts' -print0)
fi

if [[ -f application/Setups/Linux/dglan-core.service ]]; then
    cp application/Setups/Linux/dglan-core.service "$STAGE_DIR/"
fi

if [[ -f application/Setups/Linux/dglan.desktop ]]; then
    cp application/Setups/Linux/dglan.desktop "$STAGE_DIR/"
fi

OS_NAME="$(uname -sr)"
if [[ -r /etc/os-release ]]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    OS_NAME="${PRETTY_NAME:-$OS_NAME}"
fi

cat > "$STAGE_DIR/RELEASE-METADATA.txt" <<EOF
Package: $PACKAGE_NAME
Version: $VERSION
VersionTag: ${VERSION_TAG:-<none>}
BuildTimeUTC: $BUILD_TIME
GitCommit: $GIT_HASH
Architecture: $ARCH_TAG
Kernel: $(uname -srmo)
BuiltOn: $OS_NAME
QMake: $QMAKE_BIN
QtVersion: $("$QMAKE_BIN" -query QT_VERSION 2>/dev/null || echo unknown)
Protoc: $(protoc --version 2>/dev/null || echo unknown)
TranslationsCompiled: $LANGUAGE_COUNT
InstallPrefixDefault: /usr/local
Notes: Native Linux artifact; validate on the target distro and architecture before publishing widely.
EOF

cat > "$STAGE_DIR/install.sh" <<'INSTALL'
#!/usr/bin/env bash
set -euo pipefail

PREFIX="${1:-/usr/local}"
BINDIR="$PREFIX/bin"
SHAREDIR="$PREFIX/share/dglan"
APPDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ESCAPED_BINDIR="${BINDIR//&/\\&}"

sudo mkdir -p "$BINDIR" "$SHAREDIR"
sudo install -m 755 "$APPDIR/DG-LAN.Core" "$BINDIR/DG-LAN.Core"
sudo install -m 755 "$APPDIR/DG-LAN.GUI" "$BINDIR/DG-LAN.GUI"

if [[ -d "$APPDIR/styles" ]]; then
    sudo rm -rf "$SHAREDIR/styles"
    sudo cp -R "$APPDIR/styles" "$SHAREDIR/styles"
fi

if [[ -d "$APPDIR/languages" ]]; then
    sudo rm -rf "$SHAREDIR/languages"
    sudo cp -R "$APPDIR/languages" "$SHAREDIR/languages"
fi

if [[ -f "$APPDIR/dglan-core.service" ]] && command -v systemctl >/dev/null 2>&1; then
    sed "s|^ExecStart=.*|ExecStart=$ESCAPED_BINDIR/DG-LAN.Core -e|" "$APPDIR/dglan-core.service" | \
        sudo tee /etc/systemd/system/dglan-core.service >/dev/null
    sudo chmod 644 /etc/systemd/system/dglan-core.service
    sudo systemctl daemon-reload
    echo "Installed systemd unit: dglan-core.service"
fi

if [[ -f "$APPDIR/dglan.desktop" ]]; then
    DESKTOP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    mkdir -p "$DESKTOP_DIR"
    sed "s|^Exec=.*|Exec=$ESCAPED_BINDIR/DG-LAN.GUI %u|" "$APPDIR/dglan.desktop" > "$DESKTOP_DIR/dglan.desktop"
    update-desktop-database "$DESKTOP_DIR" >/dev/null 2>&1 || true
fi

echo "Installed DG-LAN under $PREFIX"
INSTALL
chmod +x "$STAGE_DIR/install.sh"

cat > "$STAGE_DIR/LINUX-QUICK-START.txt" <<'QUICKSTART'
DG-LAN Linux quick start

1. Extract the tarball.
2. Optional system install:
     ./install.sh /usr/local
3. Core only (headless):
     ./DG-LAN.Core -e
4. GUI:
     ./DG-LAN.GUI

Notes:
- The GUI needs a real X11/Wayland session.
- Linux releases are native-build artifacts. Validate on the target distro/arch.
- See BUILD.md for firewall, multicast, and packaging gotchas.
QUICKSTART

mkdir -p "$DIST_DIR"
if tar --version 2>/dev/null | grep -qi 'gnu tar'; then
    (cd "$DIST_DIR" && tar --sort=name --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner -czf "${PACKAGE_NAME}.tar.gz" "$PACKAGE_NAME")
else
    (cd "$DIST_DIR" && tar -czf "${PACKAGE_NAME}.tar.gz" "$PACKAGE_NAME")
fi

if command -v sha256sum >/dev/null 2>&1; then
    (cd "$DIST_DIR" && sha256sum "${PACKAGE_NAME}.tar.gz" > "${PACKAGE_NAME}.tar.gz.sha256")
fi

echo
echo "Tarball created: $TARBALL_PATH"
if [[ -f "$CHECKSUM_PATH" ]]; then
    echo "Checksum: $CHECKSUM_PATH"
fi
echo "Architecture: $ARCH_TAG"
echo "Version: $VERSION"

if [[ "$SKIP_PUBLISH" -eq 1 ]]; then
    echo "SkipPublish: build completed without git push or release upload."
    echo "Local Version.h changes will be restored on exit."
    exit 0
fi

require_tool gh

TAG_NAME="v$VERSION"
CURRENT_BRANCH="$(git branch --show-current)"
if [[ -z "$CURRENT_BRANCH" ]]; then
    echo "Error: refusing to publish from detached HEAD." >&2
    exit 1
fi

git add "$VERSION_FILE"
if ! git diff --cached --quiet; then
    git commit -m "chore: release $TAG_NAME (linux)"
fi
RESTORE_VERSION_FILE=0

if ! git rev-parse -q --verify "refs/tags/$TAG_NAME" >/dev/null 2>&1; then
    git tag "$TAG_NAME"
fi

git push origin "HEAD:${CURRENT_BRANCH}"
git push origin "$TAG_NAME"

if gh release view "$TAG_NAME" >/dev/null 2>&1; then
    gh release upload "$TAG_NAME" "$TARBALL_PATH" --clobber
    if [[ -f "$CHECKSUM_PATH" ]]; then
        gh release upload "$TAG_NAME" "$CHECKSUM_PATH" --clobber
    fi
else
    RELEASE_NOTES="Linux release asset for $ARCH_TAG"$'\n'"Built on: $OS_NAME"$'\n'"Commit: $GIT_HASH"
    RELEASE_ASSETS=("$TARBALL_PATH")
    if [[ -f "$CHECKSUM_PATH" ]]; then
        RELEASE_ASSETS+=("$CHECKSUM_PATH")
    fi

    gh release create "$TAG_NAME" "${RELEASE_ASSETS[@]}" \
        --title "DG-LAN $VERSION ${VERSION_TAG} (Linux)" \
        --notes "$RELEASE_NOTES" \
        --latest
fi
