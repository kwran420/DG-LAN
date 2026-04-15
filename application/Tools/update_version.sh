#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$ROOT_DIR/../.." && pwd)"
VERSION_FILE="$ROOT_DIR/../Common/Version.h"
CURRENT_DATE="$(date -u +%Y-%m-%d_%H-%M)"
CURRENT_GIT_VERSION="$(git -C "$REPO_DIR" rev-parse --short=12 HEAD)"

sed_in_place() {
   local expression="$1"

   if sed --version >/dev/null 2>&1; then
      sed -i "$expression" "$VERSION_FILE"
   else
      sed -i '' "$expression" "$VERSION_FILE"
   fi
}

sed_in_place "s/BUILD_TIME \"[^\"]*\"/BUILD_TIME \"$CURRENT_DATE\"/g"
sed_in_place "s/GIT_VERSION \"[^\"]*\"/GIT_VERSION \"$CURRENT_GIT_VERSION\"/g"
