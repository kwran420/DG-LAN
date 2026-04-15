# Linux qmake Release

## Purpose
Use this when a Windows-first qmake/Qt repo needs the smallest reliable Linux release path.

## Pattern
1. Install distro-native Qt5 dev packages, `protoc`, compiler, and `make`; do not rely on deprecated meta-packages like `qt5-default`.
2. Prefer a native tarball artifact tagged by architecture (`linux-x86_64`, `linux-arm64`, etc.) over cross-distro packaging promises.
3. In recursive qmake trees, keep the top-level Linux build serial (`make -j1`) if parallel top-level make races static archives or generated moc objects.
4. Bundle runtime-adjacent assets the app loads relative to `applicationDirPath()` (for DG-LAN: `styles/` and compiled `languages/`).
5. Separate “builds” from “release-ready”: a successful tarball is only candidate evidence until native smoke and legacy validation are green on that distro/arch.
6. Keep generated `dist/` tarballs and checksums out of source control unless the repo already tracks release artifacts; commit the wrapper/scripts/docs, not local package output.
7. Smoke the wrapper path with a no-rebuild invocation first (for DG-LAN: `./release.sh --skip-publish --skip-build`) to validate argument translation and packaging without perturbing the worktree.

## DG-LAN example
- `./build-release.sh -SkipPublish` now builds `DG-LAN.Core` and `DG-LAN.GUI` on Ubuntu 24.04 x86_64.
- The tarball includes binaries, `styles/`, compiled Qt translations, and Linux helper files (`dglan-core.service`, `dglan.desktop`, `install.sh`).
- `python3 validate.py` still fails because the old Qt test harness references removed interfaces, so Linux shipping still needs native smoke plus test-harness cleanup.
