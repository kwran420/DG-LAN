# DG-LAN — Project Context

Development reference for DG-LAN contributors and AI assistants.

## Overview

DG-LAN decentralises a master file list across a network. A master machine curates
shared folders and indexes every file; clients browse the list, download what they
need, and automatically rehost downloaded files — distributing the load across all peers.

Forked from [D-LAN](https://github.com/Ummon/D-LAN).
C++17 with Qt5, built with MSYS2 MinGW64 on Windows.

- **GitHub**: https://github.com/kwran420/DG-LAN
- **Upstream**: https://github.com/Ummon/D-LAN
- **Branch**: `master`
- **Version**: Defined in `application/Common/Version.h` (auto-incremented by release scripts)

---

## Project Structure

```
application/
  Core/          → Headless daemon (Windows service)  → DG-LAN.Core.exe
  GUI/           → Qt Widgets UI                      → DG-LAN.GUI.exe
  Common/        → Shared library (hash, network, settings, proto helpers)
  Protos/        → Protobuf definitions (pre-generated .pb.cc/.pb.h)
  Setups/Windows → Inno Setup installer (windows_setup.iss)
  Tools/         → Build helper scripts
  translations/  → .qm language files
  styles/        → UI stylesheets

dglan-api/       → Python HTTP bridge (serves file listings as JSON)
```

---

## Build System

### Prerequisites
- **MSYS2** at `C:\msys64` with MinGW64 packages: gcc, qt5-base, qt5-tools, protobuf, openssl, make (Windows)
- **Qt5 dev packages** from system package manager (Linux: apt/dnf/pacman)
- **Inno Setup 6** for Windows installer only
- **GitHub CLI** (`gh`) for publishing releases (optional)

### Build Commands

**Cross-Platform (Recommended):**
```bash
./release.sh                     # build + publish to GitHub Releases (detects platform)
./release.sh --skip-publish      # build only, no git push
./release.sh --skip-build        # re-package existing binaries
./release.sh --version 2.0.0     # override version number
```

The `release.sh` wrapper auto-detects Windows vs. Linux and dispatches to the appropriate native builder.

**Platform-Native (Advanced):**

**Windows:**
```powershell
.\build-release.ps1                  # build + publish (default)
.\build-release.ps1 -SkipPublish     # build only, no git push
.\build-release.ps1 -SkipBuild       # just rebuild the installer
.\build-release.ps1 -Version 2.0.0   # override version number
```

**Linux:**
```bash
./build-release.sh                   # build + publish (default)
./build-release.sh -SkipPublish      # build only, no git push
./build-release.sh -SkipBuild        # just re-tar binaries
./build-release.sh -Version 2.0.0    # override version number
```

**macOS (experimental):**
```bash
# manual qmake + make only (see BUILD.md)
```

### What Each Build Script Does

**Windows (`build-release.ps1`):**
1. Auto-increments patch version in `Version.h` (unless `-Version` overrides)
2. Patches `BUILD_TIME` and `GIT_VERSION` in `Version.h`
3. Builds Core + GUI via MSYS2 MinGW64 (`qmake-qt5` + `mingw32-make`)
4. Builds the Inno Setup installer (ISCC output → `DG-LAN-<ver>-Setup.exe`)
5. If publishing (default): commits `Version.h`, tags `v<version>`, pushes, creates GitHub Release with `.exe` attached

**Linux (`build-release.sh`):**
1. Auto-increments patch version in `Version.h` (unless `-Version` overrides)
2. Patches `BUILD_TIME` and `GIT_VERSION` in `Version.h`
3. Builds Core + GUI via system qmake + top-level recursive make (`-j1` for Linux reliability)
4. Creates release tarball: `dist/DG-LAN-X.Y.Z-<tag>-linux-x86_64.tar.gz` (or your arch)
5. If publishing (default): commits `Version.h`, tags `v<version>`, pushes, creates GitHub Release with `.tar.gz` attached

**Qualification note:** the script existing is not the same as Linux support being proven. Ubuntu 24.04 x86_64 now builds in this workspace, but each distro/arch still needs native build + smoke evidence before the tarball should be attached to a release.

### Build Outputs

**Windows:**
- `application/Core/output/release/DG-LAN.Core.exe`
- `application/GUI/output/release/DG-LAN.GUI.exe`
- `application/Setups/Windows/Installations/DG-LAN-<ver>-Setup.exe` ← Release artifact

**Linux:**
- `application/Core/output/release/DG-LAN.Core`
- `application/GUI/output/release/DG-LAN.GUI`
- `dist/DG-LAN-X.Y.Z-<tag>-linux-x86_64.tar.gz` ← Release artifact

### Dual-Release Strategy (v1.3+ target)

The target release shape is one Windows artifact plus one Linux artifact per version, but Linux assets should only be attached after native validation on the relevant distro/arch:

| Platform | Script | Output | Storage |
|----------|--------|--------|---------|
| Windows | `.\build-release.ps1` | `.exe` installer | GitHub Release |
| Linux x86_64 | `./build-release.sh` | `.tar.gz` tarball | GitHub Release |
| Linux ARM (RPi) | `./build-release.sh` | `.tar.gz` tarball | GitHub Release after native ARM smoke |

**Single source of truth**: `application/Common/Version.h` — both scripts read and patch it.

**Version sync**: Both `.exe` and `.tar.gz` carry identical version strings and git hashes.

**Workflow**: 
1. Windows dev runs `.\build-release.ps1` → creates GitHub Release with `.exe`
2. Linux dev runs `./build-release.sh` on the target distro/arch and uploads `.tar.gz` only after smoke coverage passes
3. Or, Phase 2 automation (GitHub Actions matrix) does both in CI

---

## Version System

Defined in `application/Common/Version.h`:
```cpp
#define VERSION "1.2.92"
#define VERSION_TAG "Alpha"
#define BUILD_TIME "..."   // patched by build-release.ps1 / build-release.sh
#define GIT_VERSION "..."      // patched by build-release.ps1 / build-release.sh
```

- `version.rc` includes `Version.h` → embeds version info into .exe resources
- The installer reads version from the built .exe via `GetStringFileInfo()`

---

## CI/CD

### Primary: Local Platform-Native Builds

All releases are built locally on their native platform via:
- **Windows**: `.\build-release.ps1` (faster, more reliable than CI for Qt5 + MSYS2 + protobuf)
- **Linux**: `./build-release.sh` (directly on target OS; no cross-compilation)
- **macOS**: manual qmake + make only (experimental)

Each script handles version bumping, tagging, and GitHub Release creation.

### Fallback: GitHub Actions

**File**: `.github/workflows/build.yml`  
**Trigger**: Push a `v*` tag OR manual `workflow_dispatch`  
**Runner**: `ubuntu-latest` (lightweight — no compile)  
**Behavior**: If a GitHub Release already exists (created by `build-release.ps1` or `build-release.sh`), CI exits. Otherwise, it creates a **draft** release so you can manually upload the installer.

### Future (Phase 2): GitHub Actions Matrix Builds

Once CI infrastructure is available, this workflow could be enhanced to:
1. Detect Windows release artifacts uploaded by local `build-release.ps1`
2. Spawn Ubuntu runner to build Linux `.tar.gz`
3. Auto-upload both to GitHub Release
4. Result: Single release tag with both `.exe` and `.tar.gz`

**Phase 2 is optional**: The current local build approach works fine and is actually simpler for contributors.

---

## Installer Details

**File**: `application/Setups/Windows/windows_setup.iss`

- Paths default to `C:/msys64/mingw64` for Qt/MinGW runtime DLLs
- Bundles: Qt5Core/Gui/Network/Widgets/Xml, ICU, MinGW runtime, OpenSSL, platform/imageformat plugins
- Output: `Installations/DG-LAN-<version><tag>-<buildtime>-Setup.exe`

---

## Auto-Update System

Clients check for updates via the GitHub Releases API.

| File | Purpose |
|------|---------|
| `GUI/UpdateChecker.h/.cpp` | Queries GitHub API, compares semver, emits `updateAvailable` |
| `GUI/UpdateDialog.h/.cpp` | Download progress dialog, launches installer with `/VERYSILENT /NORESTART` |
| `GUI/D-LAN_GUI.cpp` | Wires up auto-check on launch (3s delay) + manual check |
| `GUI/MainWindow.cpp` | Help → "Check for Updates..." menu item |

**Flow**:
1. `UpdateChecker` fetches `https://api.github.com/repos/kwran420/DG-LAN/releases?per_page=10`
2. Iterates non-draft releases, finds the highest semver tag
3. If newer than current `VERSION`, emits `updateAvailable` with the `.exe` URL
4. `UpdateDialog` downloads to temp, launches installer, app quits

**Forced Update**: When a peer reports a protocol version mismatch, `UpdateDialog` opens in forced mode (title "Update Required", no dismiss — user must update or quit).

**Note**: Uses `/releases` (list), NOT `/releases/latest` — the latter skips pre-releases, which broke auto-update.

**Settings**: `QSettings("DGLan", "DG-LAN")` → `update/check_on_launch`

---

## Modernization Timeline & Next Steps

DG-LAN is transitioning from a stable but aging C++17 + Qt5 + qmake stack to a modern, modular codebase. This section documents the current phase and upcoming work.

### Phase 0: Validation & Safety Net (v1.2.x — Current)

**Status**: ✅ Complete (April 2026)

**Deliverables:**
- ✅ `validate.py`: Unified validation entrypoint (Python baseline 59 tests, legacy C++ tests gated)
- ✅ `ARCHITECTURE.md`: Comprehensive design documentation with risk hotspots
- ✅ `OPERATIONS.md`: Windows Service setup, logging, troubleshooting
- ✅ Peer lifecycle signals: `peerBecomesUnavailable` for proactive cleanup
- ✅ Occupancy refactoring: Hash-keyed peer bookkeeping (safer than raw pointers)

**Quality baseline:** Python bridge (59 tests, security-first) is canonical; C++ should match

### Phase 1: Test Infrastructure & Hardening (v1.3 — Weeks 2–3)

**In progress / Upcoming:**

- 🎯 **Re-enable DownloadManager tests**: Now that occupancy refactoring landed
- 🎯 **Add RemoteControlManager tests**: GUI ↔ Core protocol coverage
- 🎯 **Add HttpServer integration tests**: Range parsing, CORS, peer redirects
- 🎯 **Log rotation**: Implement automatic cleanup (24/7 deployments unbounded logs)
- 🎯 **Slow client detection**: HTTP timeout to prevent file descriptor exhaustion
- 🎯 **CODE-STYLE.md**: ✅ Created (April 2026)
- 🎯 **TESTING.md**: ✅ Expanded (April 2026)

**Expected outcome**: C++ test coverage parity with Python (80%+), CI gates enabled

### Phase 2: GUI Dead Code Removal (v1.3 — Weeks 1–2)

**In progress:**

- 🎯 Chat feature: 34 KB, never instantiated → remove
- 🎯 Emoticons: 13.8 KB, depends on Chat → remove
- 🎯 Activity widget: 9.3 KB, orphaned → remove
- 🎯 Hashing widget: 11.8 KB, orphaned → remove
- 🎯 Uploads widget: Mark as "Not Implemented" (may be planned for v2.0)

**Expected outcome**: Cleaner codebase, faster builds, reduced maintenance surface

### Phase 3: Modularization (v1.4+)

**Design in progress:**

- 🎯 **God class refactoring**: Split UDPListener, RemoteConnection, Cache
- 🎯 **Control plane boundary**: Isolate RemoteConnection (command dispatch)
- 🎯 **Data plane optimization**: Separate file serving from peer bookkeeping

**Expected outcome**: Clearer module boundaries, easier testing, safer refactoring

### Phase 4: Toolchain Modernization (v2.0+)

**Long-term:**

- 🎯 **CMake**: Replace qmake for cross-platform builds
- 🎯 **Qt6**: Upgrade from Qt5 (modern signals, better threading, Qt6 QML compatibility)
- 🎯 **Smart pointers**: Replace raw pointers with QSharedPointer, std::unique_ptr
- 🎯 **TLS/HTTPS**: Peer-to-peer security (currently network-isolation-based)
- 🎯 **C++20**: Coroutines, modules if feasible

**Blocking dependencies**: None in v1.2–v1.3 (safe to keep Qt5 + qmake)

### Key Constraints & Assumptions

- **Windows-first**: All releases built locally via `build-release.ps1` (faster, more reliable than CI)
- **Python bridge is quality baseline**: 59 tests cover protocol, security, edge cases — C++ should reach parity
- **No breaking changes**: All modernization happens on feature branches, verified against validate.py
- **Honest about timeline**: No promises on CMake/Qt6 until v2.0 planning is complete

---

## Architecture Notes

### Core ↔ GUI Connection
- GUI connects to Core via TCP on localhost:59485
- Communication uses protobuf messages with `Common::MessageHeader` type codes
- `RemoteConnection` (in `Core/RemoteControlManager`) handles all GUI → Core commands
- GUI can connect to a remote Core via Settings → Core Address

### Peer Discovery (fallback chain)
1. **Multicast** (`224.0.0.1:59486`) — always
2. **Directed broadcast** (`x.x.x.255`) — fallback ~2s
3. **Subnet scan** (`/24`) — fallback ~4s
4. **Gossip / PEX** — continuous after first peer contact

### Peer-to-Peer Browsing
- **Local browse**: `RemoteConnection` → `FileManager::getEntries()` (sync, mutex-protected)
- **Remote browse**: GUI → `RemoteConnection` → `Peer::getEntries()` → `CORE_GET_ENTRIES` over network → remote `GetEntriesResult` waits for scan → sends `CORE_GET_ENTRIES_RESULT`

### Master / Client Architecture
- **Master mode** (`client_mode = false`): indexes ALL files in shared directories, serves the canonical file list
- **Client mode** (`client_mode = true`): only indexes downloaded files and files already in shared folders (selective rehosting)
- At least one machine must run as master; clients browse and rehost from that list
- Multiple masters can coexist — the network elects one to serve the canonical index

### File Scanning & Caching
- `FileUpdater` thread continuously scans shared directories
- `Cache` stores the directory tree; `Directory*` raw pointers used throughout
- `file_cache.bin`: protobuf binary saved every 60s via `timerPersistCache`
- **Master**: indexes everything in shared folders (full scan)
- **Client**: selective rehosting — watcher-driven rescans skip genuinely new files; only downloads and existing files are indexed

### GUI Layout
- **MainWindow**: Central MdiArea + Log dock (always visible) + StatusBar
- **NetworkWidget**: Unified file index — browse the master file list with columns: Name, Size, Status, Queue #, Progress, DL Speed, UL Speed, Peers
- **PeersDock**: Peer list with priority (High/Normal/Low) and connection speed
- **Settings**: Menu bar → dialog (not tabs)
- **ScrollingNotification**: Marquee banner when update available

---

## Key Files

| File | Purpose |
|------|---------|
| `application/Common/Version.h` | Version macros |
| `application/Common/version.rc` | Windows resource file for .exe version info |
| `application/Core.pro` | qmake project → `Makefile-Core` |
| `application/GUI.pro` | qmake project → `Makefile-GUI` |
| `application/GUI/Browse/NetworkWidget.h/.cpp` | Unified file index widget |
| `application/GUI/Peers/PeersDock.h/.cpp` | Peer list with priority |
| `application/GUI/UpdateChecker.h/.cpp` | Auto-update version checker |
| `application/GUI/UpdateDialog.h/.cpp` | Update download + install dialog |
| `application/GUI/Log/LogModel.h/.cpp` | Activity log model |
| `application/Core/NetworkListener/priv/UDPListener.h/.cpp` | Discovery (multicast, broadcast, scan, gossip) |
| `application/Core/FileManager/priv/FileUpdater/FileUpdater.h/.cpp` | File scanning + selective rehosting |
| `application/Setups/Windows/windows_setup.iss` | Inno Setup installer script |
| `.github/workflows/build.yml` | CI fallback (draft release) |
| `build-release.ps1` | Primary build + publish script |
