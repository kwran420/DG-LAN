#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

FORWARD_ARGS=()
PUBLISH=0

usage() {
    cat <<'USAGE'
Usage: ./build-linux.sh [--publish] [--skip-build] [--version X.Y.Z]

Compatibility wrapper around build-release.sh.
- Default behavior matches the old script: build locally without publishing
- Use --publish to push/tag/create the GitHub release
USAGE
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --publish)
            PUBLISH=1
            shift
            ;;
        --skip-build)
            FORWARD_ARGS+=(-SkipBuild)
            shift
            ;;
        --version)
            if [[ -z "${2:-}" ]]; then
                echo "Error: --version requires a value" >&2
                exit 1
            fi
            FORWARD_ARGS+=(-Version "$2")
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

if [[ "$PUBLISH" -eq 0 ]]; then
    FORWARD_ARGS+=(-SkipPublish)
fi

exec "$ROOT_DIR/build-release.sh" "${FORWARD_ARGS[@]}"
