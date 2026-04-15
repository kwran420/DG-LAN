#!/usr/bin/env bash
# Generates makefiles and compiles all components and their tests.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

VALIDATION_PROJECTS=(
   Common
   Common/TestsCommon
   Common/LogManager
   Common/RemoteCoreController
   Core/FileManager
   Core/FileManager/TestsFileManager
   Core/PeerManager
   Core/PeerManager/TestsPeerManager
   Core/UploadManager
   Core/DownloadManager
   Core/DownloadManager/TestsDownloadManager
   Core/NetworkListener
   Core/ChatSystem
   Core/HttpServer
   Core/RemoteControlManager
   Core
   GUI
)

LEGACY_PROJECTS=(
   "${VALIDATION_PROJECTS[@]}"
   Tools/PasswordHasher
)

EXPERIMENTAL_TEST_PROJECTS=(
)

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
      MAKE_BIN="$(command -v mingw32-make.exe 2>/dev/null || command -v mingw32-make 2>/dev/null || true)"
      ;;
esac

QMAKE_BIN="$(command -v qmake-qt5 2>/dev/null || command -v qmake 2>/dev/null || true)"
if [[ -z "$QMAKE_BIN" ]]; then
   echo "Error: qmake not found." >&2
   exit 1
fi

if [[ -z "${MAKE_BIN:-}" ]]; then
   echo "Error: make tool not found." >&2
   exit 1
fi

CLEAN_COMMAND=0
PROF=""
UPDATE_VERSION=0
MANIFEST="validation"

usage() {
   cat <<'USAGE'
Usage : ./3.compile_all_components.sh [--prof] [--clean] [--update-version] [--validation|--legacy]
 --prof            Activate profiling
 --clean           Clean temporary objects before each compilation
 --update-version  Refresh BUILD_TIME/GIT_VERSION in Version.h
 --validation      Build the Linux validation graph (default)
 --legacy          Include optional legacy utilities as well
USAGE
}

while [[ $# -gt 0 ]]; do
   case "$1" in
      --prof)
         PROF="prof"
         echo "Profiling activated"
         shift
         ;;
      --clean)
         CLEAN_COMMAND=1
         echo "Clean activated"
         shift
         ;;
      --update-version)
          UPDATE_VERSION=1
          echo "Version metadata update activated"
          shift
          ;;
      --validation)
         MANIFEST="validation"
         shift
         ;;
      --legacy)
         MANIFEST="legacy"
         shift
         ;;
      --with-stale-tests|--with-experimental-tests)
         INCLUDE_EXPERIMENTAL_TESTS=1
         echo "Including experimental legacy tests"
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

if [[ "$UPDATE_VERSION" -eq 1 ]]; then
   (cd Tools && ./update_version.sh)
fi

case "$MANIFEST" in
   validation)
      PROJECTS=("${VALIDATION_PROJECTS[@]}")
      ;;
   legacy)
      PROJECTS=("${LEGACY_PROJECTS[@]}")
      ;;
   *)
      echo "Error: unknown manifest '$MANIFEST'" >&2
      exit 1
      ;;
esac

echo "Using manifest: $MANIFEST"

# Force recompilation of version resources and About dialog where applicable.
rm -f Core/.tmp/release/version_res.o
rm -f GUI/.tmp/release/version_res.o
rm -f GUI/.tmp/release/DialogAbout.o

NB_PROC="${DGLAN_MAKE_JOBS:-}"
if [[ -z "$NB_PROC" ]]; then
   NB_PROC="$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

for project_path in "${PROJECTS[@]}"; do
   project_name="$(basename "$project_path")"
   echo "Compiling $project_name.."

   pushd "$project_path" >/dev/null
   "$QMAKE_BIN" "${project_name}.pro" -r -spec "$SPEC" "CONFIG+=release $PROF"
   if [[ "$CLEAN_COMMAND" -eq 1 ]]; then
      "$MAKE_BIN" release-clean -w || echo "nothing to clean"
   fi
   "$MAKE_BIN" -w -j"$NB_PROC"
   popd >/dev/null
done

echo "Compilation finished successfully"
