# DG-LAN — Project Context

Development reference for DG-LAN contributors and AI assistants.

## Overview

DG-LAN is a decentralized LAN file-sharing application. Forked from [D-LAN](https://github.com/Ummon/D-LAN).  
C++17 with Qt5, built with MSYS2 MinGW64 on Windows.

- **GitHub**: https://github.com/kwran420/DG-LAN
- **Upstream**: https://github.com/Ummon/D-LAN
- **Branch**: `master`
- **Version**: Defined in `application/Common/Version.h` (auto-incremented by `build-release.ps1`)

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
- **MSYS2** at `C:\msys64` with MinGW64 packages: gcc, qt5-base, qt5-tools, protobuf, openssl, make
- **Inno Setup 6** for building the installer
- **GitHub CLI** (`gh`) for publishing releases (optional)

### Build Commands

```powershell
.\build-release.ps1                  # build + publish (default)
.\build-release.ps1 -SkipPublish     # build only, no git push
.\build-release.ps1 -SkipBuild       # just rebuild the installer
.\build-release.ps1 -Version 2.0.0   # override version number
```

### What `build-release.ps1` Does
1. Auto-increments patch version in `Version.h` (unless `-Version` overrides)
2. Patches `BUILD_TIME` and `GIT_VERSION` in `Version.h`
3. Builds Core + GUI via MSYS2 MinGW64 (`qmake-qt5` + `mingw32-make`)
4. Builds the Inno Setup installer (ISCC output → `installer_log.txt`)
5. If publishing (default): commits `Version.h`, tags `v<version>`, pushes, creates GitHub Release with installer attached

### Build Outputs
- `application/Core/output/release/DG-LAN.Core.exe`
- `application/GUI/output/release/DG-LAN.GUI.exe`
- `application/Setups/Windows/Installations/DG-LAN-<ver><tag>-<buildtime>-Setup.exe`

---

## Version System

Defined in `application/Common/Version.h`:
```cpp
#define VERSION "1.2.92"
#define VERSION_TAG "Alpha"
#define BUILD_TIME "..."   // patched by build-release.ps1
#define GIT_VERSION "..."      // patched by build-release.ps1
```

- `version.rc` includes `Version.h` → embeds version info into .exe resources
- The installer reads version from the built .exe via `GetStringFileInfo()`

---

## CI/CD

### Primary: Local Build
All releases are built locally via `.\build-release.ps1`. This is faster and more reliable than CI for a Qt5 + MSYS2 + protobuf project.

### Fallback: GitHub Actions
**File**: `.github/workflows/build.yml`  
**Trigger**: Push a `v*` tag OR manual `workflow_dispatch`  
**Runner**: `ubuntu-latest` (lightweight — no compile)  
**Behavior**: If a GitHub Release already exists (created by `build-release.ps1`), CI exits. Otherwise, it creates a **draft** release so you can manually upload the installer.

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

### File Scanning & Caching
- `FileUpdater` thread continuously scans shared directories
- `Cache` stores the directory tree; `Directory*` raw pointers used throughout
- `file_cache.bin`: protobuf binary saved every 60s via `timerPersistCache`
- Selective rehosting: watcher-driven rescans skip genuinely new files; only downloads and existing files are indexed

### GUI Layout
- **MainWindow**: Central MdiArea + Log dock (always visible) + StatusBar
- **NetworkWidget**: Unified file index — all peers' files in one table with columns: Name, Size, Status, Queue #, Progress, DL Speed, UL Speed, Peers
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
