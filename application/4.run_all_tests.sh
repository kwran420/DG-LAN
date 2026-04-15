#!/usr/bin/env bash
# Runs all wired test binaries.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

if [[ "$(uname -s)" == "Linux" || "$(uname -s)" == "Darwin" ]]; then
   EXTENSION=""
else
   EXTENSION=".exe"
fi

TESTS=(
   Common/TestsCommon/output/release/TestsCommon$EXTENSION
   Core/FileManager/TestsFileManager/output/release/TestsFileManager$EXTENSION
   Core/PeerManager/TestsPeerManager/output/release/TestsPeerManager$EXTENSION
   Core/DownloadManager/TestsDownloadManager/output/release/TestsDownloadManager$EXTENSION
)

PROFILE="validation"

usage() {
   cat <<'USAGE'
Usage : ./4.run_all_tests.sh [--validation|--legacy]
 --validation        Run the wired validation suites (default)
 --legacy            Compatibility alias; currently runs the same wired suites
USAGE
}

while [[ $# -gt 0 ]]; do
   case "$1" in
      --validation)
         PROFILE="validation"
         shift
         ;;
      --legacy)
         PROFILE="legacy"
         shift
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

echo "Using test profile: $PROFILE"

for test_path in "${TESTS[@]}"; do
   if [[ ! -x "$test_path" ]]; then
      echo "Missing test binary: $test_path" >&2
      exit 1
   fi

   test_dir="$(dirname "$test_path")"
   test_name="$(basename "$test_path")"
   echo "Executing $test_name.."
   (
      cd "$test_dir"
      "./$test_name"
   )
done

echo "All tests finished successfully"
