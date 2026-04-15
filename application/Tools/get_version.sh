#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="$ROOT_DIR/../Common/Version.h"

VERSION="$(sed -n 's/#define VERSION "\(.*\)"/\1/p' "$VERSION_FILE")"
VERSION_TAG="$(sed -n 's/#define VERSION_TAG "\(.*\)"/\1/p' "$VERSION_FILE")"

printf '%s%s\n' "$VERSION" "$VERSION_TAG"
