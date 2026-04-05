# DG-LAN Project Context

## What is this?
DG-LAN is a fork of [D-LAN](https://github.com/Ummon/D-LAN) — a decentralized LAN file sharing application.  
C++ with Qt5, built with MSYS2 MinGW64 toolchain.

- **GitHub**: https://github.com/kwran420/DG-LAN.git
- **Upstream**: https://github.com/Ummon/D-LAN.git
- **Branch**: `master`
- **Current version**: 1.2.3 Alpha (`application/Common/Version.h`)

---

## Project Structure

```
application/
  Core/          → Headless service/daemon → DG-LAN.Core.exe
  GUI/           → Qt Widgets UI           → DG-LAN.GUI.exe
  Common/        → Shared library (hash, network, settings, etc.)
  Protos/        → Protobuf definitions (pre-generated .pb.cc/.pb.h)
  Setups/Windows → Inno Setup installer (windows_setup.iss)
  Tools/         → Build helper scripts
  translations/  → .qm language files
  styles/        → UI stylesheets
```

---

## Build System

### Requirements
- **MSYS2** installed at `C:\msys64` with MinGW64 packages:
  - `mingw-w64-x86_64-gcc`
  - `mingw-w64-x86_64-qt5-base`
  - `mingw-w64-x86_64-qt5-tools`
  - `mingw-w64-x86_64-protobuf`
  - `mingw-w64-x86_64-openssl`
  - `mingw-w64-x86_64-make`
- **Inno Setup 6** for building the installer

### Local Build (PowerShell)
```powershell
.\build-release.ps1                        # build with version from Version.h
.\build-release.ps1 -Version 1.3.0         # override version
.\build-release.ps1 -SkipBuild             # just rebuild the installer
.\build-release.ps1 -Publish               # build + tag + push + create GitHub Release
.\build-release.ps1 -Version 1.3.0 -Publish  # override version + publish
```

### Manual Build (MSYS2 bash)
```bash
# IMPORTANT: Must use MINGW64 subsystem for qmake to be in PATH
export MSYSTEM=MINGW64

cd /c/Dev/DG-LAN/application

# Build Core
qmake-qt5 Core.pro -r -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile-Core -j$(nproc)

# Build GUI
qmake-qt5 GUI.pro -r -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile-GUI -j$(nproc)
```

### Build Outputs
- `application/Core/output/release/DG-LAN.Core.exe`
- `application/GUI/output/release/DG-LAN.GUI.exe`
- `application/Setups/Windows/Installations/DG-LAN-<ver>-Setup.exe`

---

## Version System

Defined in `application/Common/Version.h`:
```cpp
#define VERSION "1.2.3"
#define VERSION_TAG "Alpha"
#define BUILD_TIME "..."       // patched by build-release.ps1 at build time
#define GIT_VERSION "..."     // patched by build-release.ps1 at build time
```

- `version.rc` includes Version.h → embeds version info into .exe resources
- The installer (.iss) reads version from the built .exe via `GetStringFileInfo()`

---

## Release & CI/CD

### How to Release (one command)
```powershell
.\build-release.ps1 -Publish
```
This does everything:
1. Patches `Version.h` with build time + git hash
2. Builds Core + GUI via MSYS2 MinGW64
3. Builds the Inno Setup installer
4. Commits pending changes, creates a git tag (`vX.Y.Z`), pushes to origin
5. Creates a GitHub Release and uploads the `.exe` installer

Clients auto-update by checking GitHub Releases (see Auto-Update below).

### Why local builds?
Building on GitHub Actions required MSYS2 + Qt5 + protobuf setup (~10 min).
Local builds are faster and more reliable. The CI workflow now only exists as
a lightweight fallback — if a tag is pushed without a release, CI creates a
draft release on `ubuntu-latest` (no compile) so you can manually upload.

### CI Workflow
**File**: `.github/workflows/build.yml`  
**Trigger**: Push a `v*` tag OR manual `workflow_dispatch`  
**Runner**: `ubuntu-latest` (lightweight — no build, no MSYS2)  
**Behaviour**:
- If a GitHub Release already exists for the tag (created by `-Publish`), CI exits immediately
- If no release exists, CI creates a **draft** release — you then upload the .exe manually:
  ```bash
  gh release upload v1.3.0 path/to/DG-LAN-Setup.exe
  gh release edit v1.3.0 --draft=false
  ```

---

## Installer (.iss) Details

**File**: `application/Setups/Windows/windows_setup.iss`

- `QtDir` / `MingwDir` default to `C:/msys64/mingw64` (correct for local builds)
- Bundles: Qt5Core/Gui/Network/Widgets/Xml, ICU, MinGW runtime, OpenSSL, platform/imageformat plugins
- ICU version currently 78 — may change with MSYS2 updates
- Output: `Installations/DG-LAN-<version><tag>-<buildtime>-Setup.exe`

---

## Auto-Update System

Clients check for updates via the GitHub Releases API.

**Files**:
- `application/GUI/UpdateChecker.h/.cpp` — queries GitHub API, compares versions
- `application/GUI/UpdateDialog.h/.cpp` — download progress dialog, launches installer
- `application/GUI/D-LAN_GUI.cpp` — wires up auto-check on launch + manual check
- `application/GUI/MainWindow.cpp` — Help → "Check for Updates..." menu item

**How it works**:
1. On launch (3s delay, if auto-check enabled) or via Help menu / tray icon
2. `UpdateChecker` fetches `https://api.github.com/repos/kwran420/DG-LAN/releases?per_page=10`
3. Iterates all non-draft releases, finds the highest semver tag
4. If newer than current `VERSION`, emits `updateAvailable` with the `.exe` download URL
5. `UpdateDialog` downloads the `.exe` to temp, launches it with `/VERYSILENT /NORESTART`
6. App quits, Inno Setup installs over the existing installation

**Important**: The endpoint is `/releases` (list), NOT `/releases/latest`.
This is intentional — `/releases/latest` skips pre-releases, which broke
auto-update when CI was publishing with `--prerelease`.

**Settings**: Auto-check on launch is stored in `QSettings("DGLan", "DG-LAN")` → `update/check_on_launch`

---

## Key Files

| File | Purpose |
|------|---------|
| `application/Common/Version.h` | Version macros (VERSION, VERSION_TAG, BUILD_TIME, GIT_VERSION) |
| `application/Common/version.rc` | Windows resource file embedding version into .exe |
| `application/Core.pro` | qmake project → Makefile-Core |
| `application/GUI.pro` | qmake project → Makefile-GUI |
| `application/Setups/Windows/windows_setup.iss` | Inno Setup installer script |
| `.github/workflows/build.yml` | CI fallback — creates draft release if `-Publish` wasn't used |
| `build-release.ps1` | Local build + optional publish to GitHub Releases (`-Publish`) |
| `application/GUI/UpdateChecker.cpp` | GitHub API client — checks for new releases |
| `application/GUI/UpdateDialog.cpp` | Download + install UI for auto-update |

---

## Git Remotes
```
origin    https://github.com/kwran420/DG-LAN.git
upstream  https://github.com/Ummon/D-LAN.git
```

## Important Notes
- Project was moved from OneDrive to `C:\Dev\DG-LAN` — spaces in the OneDrive path broke MSYS2 bash
- `$env:MSYSTEM = "MINGW64"` must be set before calling `bash --login` from PowerShell
- The `build-release.ps1` script handles Windows→MSYS2 path conversion automatically
