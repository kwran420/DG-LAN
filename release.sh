#!/usr/bin/env bash
#
# Cross-platform release entrypoint for DG-LAN
#
# Works on both Windows (Git Bash/MSYS2) and Linux.
# Dispatches to the native platform-specific builder:
#   - Windows → build-release.ps1 (via PowerShell)
#   - Linux   → build-release.sh
#
# Usage:
#   ./release.sh                      # build + publish (default)
#   ./release.sh --skip-publish       # build locally without git push
#   ./release.sh --skip-build         # re-package existing binaries
#   ./release.sh --version X.Y.Z      # override version
#

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

usage() {
    cat <<'USAGE'
Cross-platform DG-LAN release builder

Usage:
  ./release.sh [OPTIONS]

Options:
  --skip-publish       Build locally without git push or GitHub Release
  --skip-build         Re-package existing binaries without rebuilding
  --version X.Y.Z      Override the version number
  -h, --help           Show this help

Examples:
  ./release.sh                     # Build + publish to GitHub Releases (default)
  ./release.sh --skip-publish      # Build only, no git push
  ./release.sh --version 2.0.0     # Build + publish with explicit version

Platform Support:
  - Windows: Calls build-release.ps1 via PowerShell
  - Linux:   Calls build-release.sh
USAGE
}

# Parse arguments
ARGS=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        --skip-publish|--skip-build|--version)
            ARGS+=("$1")
            if [[ "$1" == "--version" ]]; then
                if [[ -z "${2:-}" ]]; then
                    echo "Error: --version requires a value" >&2
                    exit 1
                fi
                ARGS+=("$2")
                shift
            fi
            shift
            ;;
        *)
            echo "Error: Unknown option: $1" >&2
            usage
            exit 1
            ;;
    esac
done

# Detect platform
case "$(uname -s)" in
    Linux*)
        PLATFORM="Linux"
        ;;
    Darwin*)
        echo "Error: release.sh does not support macOS packaging." >&2
        echo "Use the manual macOS build path documented in BUILD.md." >&2
        exit 1
        ;;
    CYGWIN*|MINGW*|MSYS*)
        PLATFORM="Windows"
        ;;
    *)
        echo "Error: Unsupported platform: $(uname -s)" >&2
        echo "Supported: Linux, Windows (Git Bash/MSYS2)" >&2
        exit 1
        ;;
esac

echo "=== DG-LAN Cross-Platform Release Builder ==="
echo "Platform detected: $PLATFORM"
echo

# Convert arguments to native format
NATIVE_ARGS=()
for arg in "${ARGS[@]}"; do
    case "$arg" in
        --skip-publish)
            if [[ "$PLATFORM" == "Windows" ]]; then
                NATIVE_ARGS+=("-SkipPublish")
            else
                NATIVE_ARGS+=("-SkipPublish")
            fi
            ;;
        --skip-build)
            if [[ "$PLATFORM" == "Windows" ]]; then
                NATIVE_ARGS+=("-SkipBuild")
            else
                NATIVE_ARGS+=("-SkipBuild")
            fi
            ;;
        --version)
            if [[ "$PLATFORM" == "Windows" ]]; then
                NATIVE_ARGS+=("-Version")
            else
                NATIVE_ARGS+=("-Version")
            fi
            ;;
        *)
            NATIVE_ARGS+=("$arg")
            ;;
    esac
done

# Dispatch to platform-specific builder
case "$PLATFORM" in
    Windows)
        if ! command -v powershell.exe >/dev/null 2>&1; then
            echo "Error: PowerShell not found. Required for Windows builds." >&2
            exit 1
        fi

        # Convert Unix path to Windows path for PowerShell
        WIN_SCRIPT="$ROOT_DIR/build-release.ps1"
        if [[ "$WIN_SCRIPT" =~ ^/([a-z])/(.+)$ ]]; then
            # Git Bash format: /c/path → C:\path
            WIN_SCRIPT="${BASH_REMATCH[1]^^}:\\${BASH_REMATCH[2]//\//\\}"
        fi

        echo "Calling: powershell.exe -ExecutionPolicy Bypass -File \"$WIN_SCRIPT\" ${NATIVE_ARGS[*]}"
        exec powershell.exe -ExecutionPolicy Bypass -File "$WIN_SCRIPT" "${NATIVE_ARGS[@]}"
        ;;

    Linux)
        SCRIPT="$ROOT_DIR/build-release.sh"
        if [[ ! -x "$SCRIPT" ]]; then
            echo "Error: $SCRIPT not found or not executable" >&2
            exit 1
        fi

        echo "Calling: $SCRIPT ${NATIVE_ARGS[*]}"
        exec "$SCRIPT" "${NATIVE_ARGS[@]}"
        ;;
esac
