# DG-LAN — Build Instructions

DG-LAN decentralises a master file list across a network. It is built with **MSYS2 MinGW64** on Windows (the primary platform) and has an **experimental native Linux build path** based on system Qt5. macOS builds are possible but untested.

**Platform support:**
- 🔵 **Windows (x86_64)**: Primary platform — full support, auto-update, `.exe` installer
- 🟢 **Linux (x86_64, ARM)**: Experimental — native tarball path exists, but support must be proven per distro/arch
- 🟡 **macOS**: Possible (not regularly tested) — manual qmake path only

---

## Windows Build (Primary)

### Prerequisites

1. **[MSYS2](https://www.msys2.org/)** installed at `C:\msys64` (default)
2. Open an **MSYS2 MinGW64** shell and install packages:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-qt5-base \
             mingw-w64-x86_64-qt5-tools mingw-w64-x86_64-protobuf \
             mingw-w64-x86_64-openssl mingw-w64-x86_64-make
   ```
3. **[Inno Setup 6](https://jrsoftware.org/isinfo.php)** — for building the Windows installer
4. **[GitHub CLI](https://cli.github.com/)** (`gh`) — for publishing releases (optional)

---

## Validating Your Build

After building, run the repo's unified validation entrypoint:

```bash
python3 validate.py
```

This runs:
1. **Python bridge tests** (59 automated tests) — Always attempted
2. **Desktop C++ tests** (wired legacy Qt/C++ suites) — If `bash`, `qmake`, and `protoc` are available

**Exit codes:**
- `0` — All validations passed (or all attempted layers passed)
- `1` — At least one validation failed
- `2` — At least one required toolchain is missing (BLOCKED status; not a failure)

For details, see [TESTING.md](TESTING.md).

---

## One-Command Build (Cross-Platform)

### Unified Release Command

The canonical release command works on both Windows and Linux:

```bash
./release.sh --skip-publish   # build only — no git push
```

This wrapper:
- Auto-detects your platform (Windows/Linux)
- Dispatches to the native builder (`build-release.ps1` on Windows, `build-release.sh` on Linux)
- Normalizes flag syntax across platforms
- Patches `Version.h` with the build timestamp and git hash
- Auto-increments the patch version (e.g. 1.2.85 → 1.2.86)

**Windows Output:**
- `application/Core/output/release/DG-LAN.Core.exe`
- `application/GUI/output/release/DG-LAN.GUI.exe`
- `application/Setups/Windows/Installations/DG-LAN-<version>-Setup.exe`

**Linux Output:**
- `application/Core/output/release/DG-LAN.Core`
- `application/GUI/output/release/DG-LAN.GUI`
- `dist/DG-LAN-<version>-<tag>-linux-x86_64.tar.gz`

### Build + Publish (Release)

```bash
./release.sh               # default: build + commit + tag + push + GitHub Release
```

This does everything above, plus:
- Commits `Version.h`, tags with `v<version>`, pushes to origin
- Creates/updates a GitHub Release and uploads the platform-specific asset

### Unified Flags

| Flag | Effect |
|------|--------|
| *(no flags)* | Build + publish to GitHub Releases |
| `--skip-publish` | Build locally, no git push or release |
| `--skip-build` | Skip compilation, re-package existing binaries |
| `--version 2.0.0` | Override the version number |

### Platform-Native Builders (Advanced)

For direct control, use the platform-specific scripts:

**Windows (PowerShell):**
```powershell
.\build-release.ps1 -SkipPublish   # build only
```

**Linux (Bash):**
```bash
./build-release.sh -SkipPublish    # build only
```

The native scripts use PowerShell-style flags on Windows (`-SkipPublish`) and support the same flags on Linux. Use `./release.sh` for a consistent cross-platform experience on Windows and Linux.

### Manual Build (MSYS2 Shell)

If you prefer manual control, open an **MSYS2 MinGW64** shell:

```bash
export MSYSTEM=MINGW64
cd /c/Dev/DG-LAN/application

# Build Core
qmake-qt5 Core.pro -r -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile-Core -j$(nproc)

# Build GUI
qmake-qt5 GUI.pro -r -spec win32-g++ "CONFIG+=release"
mingw32-make -f Makefile-GUI -j$(nproc)
```

---

## Linux Build (Experimental)

DG-LAN builds and runs natively on Linux x86_64. ARM (Raspberry Pi) is structurally supported but requires native compilation on the target device. All Linux releases use distro Qt5/protobuf packages.

### Prerequisites

Choose your distro and install Qt5 dev packages and build tools:

**Ubuntu 20.04+ / Debian 11+:**
```bash
sudo apt-get update
sudo apt-get install -y qtbase5-dev qt5-qmake qtchooser \
    qttools5-dev-tools libprotobuf-dev protobuf-compiler build-essential git
```

**Fedora 34+ / RHEL 8+:**
```bash
sudo dnf install -y qt5-qtbase-devel qt5-qttools-devel \
    protobuf-devel protobuf-compiler gcc-c++ make git
```

**Raspberry Pi OS (Debian-based):**
```bash
sudo apt-get update
sudo apt-get install -y qtbase5-dev qt5-qmake qtchooser \
    qttools5-dev-tools libprotobuf-dev protobuf-compiler build-essential git
```

**Verify installation:**
```bash
qmake --version          # Should show Qt 5.x
protoc --version         # Should show 3.x+
gcc --version            # Should show 9.x+
```

### One-Command Build (Linux)

The cross-platform wrapper works on Linux too:

```bash
./release.sh --skip-publish      # build only (no publish)
./release.sh                     # build + tag + GitHub release
./release.sh --version 2.0.0     # override version
```

Or use the native Linux builder directly:

```bash
./build-release.sh -SkipPublish      # build only (no publish)
./build-release.sh                   # build + tag + GitHub release
./build-release.sh -Version 2.0.0    # override version
./build-release.sh -SkipBuild        # package from last build
```

Both commands:
- Auto-detect your architecture (x86_64, aarch64, armhf)
- Patch `Version.h` with build timestamp and git hash
- Build Core and GUI via qmake + make
- Create a release tarball: `dist/DG-LAN-X.Y.Z-Alpha-linux-x86_64.tar.gz`
- Restore `Version.h` after local `-SkipPublish` builds so validation/builds do not leave the worktree dirty

**Output:**
```
application/Core/output/release/DG-LAN.Core
application/GUI/output/release/DG-LAN.GUI
dist/DG-LAN-X.Y.Z-Alpha-linux-x86_64.tar.gz  ← Release tarball
```

**Legacy wrapper:** `build-linux.sh` remains as a compatibility wrapper that forwards to `build-release.sh` with `-SkipPublish` by default.

### Manual Build (Linux)

If you prefer step-by-step control:

```bash
cd application

# Build Core
qmake Core.pro -r -spec linux-g++
make -f Makefile-Core -j1

# Build GUI
qmake GUI.pro -r -spec linux-g++
make -f Makefile-GUI -j1
```

### Linux Gotchas & Mitigation

| Issue | Cause | Solution |
|-------|-------|----------|
| **LTO internal compiler error** | GCC ≥ 13 ICE when linking LTO objects + static archives | LTO is disabled by default on Linux builds (see `common.pri`). Opt in with `DEFINES+=ENABLE_LTO` only if your GCC version is known-good. |
| **Multicast discovery fails** | Distro-specific routing; some ufw rules block 224.0.0.0/4 | See [Multicast Routing](#multicast-routing-linux) below |
| **GUI won't start** | No X11/Wayland display server (headless systems) | Just run Core; GUI requires display. Core works headless. |
| **Service install differs by distro** | The tarball ships a systemd unit, but older repo packaging still uses SysV init scripts | Validate the systemd path on the target distro; do not assume old Debian packaging matches current tarball flow |
| **Config file not found** | Linux uses `~/.config/DGLan/` (XDG), not Windows paths | QSettings handles this automatically; just works |
| **Port 59485+ already in use** | Another service binding same ports | Change via Settings GUI or `~/.config/DGLan/DG-LAN.conf` |
| **qmake package name varies** | Ubuntu/Debian, Fedora/RHEL, and Raspberry Pi package Qt5 differently | Install the distro-native Qt5 development packages above; do not rely on `qt5-default` on newer Debian/Ubuntu releases |
| **protoc not found** | protobuf-compiler not installed | Run package manager install |
| **Experimental DownloadManager suite is stale** | `--with-stale-tests` opts into an older Qt test harness that still needs modernization | Use default `python3 validate.py` / `bash 4.run_all_tests.sh` for the wired suites; only opt in when actively fixing DownloadManager coverage |

### Multicast Routing (Linux)

DG-LAN peers discover each other via multicast on `224.0.0.1:59486`. Some Linux distributions require explicit routing.

**Check if multicast routing is enabled:**
```bash
ip route show table local | grep 224.0.0.0
```

If you see a line with `224.0.0.0/4 dev eth0`, you're good.

**If missing, add it:**

**One-time (temporary, lost on reboot):**
```bash
sudo ip route add 224.0.0.0/4 dev eth0
# Replace eth0 with your network interface (ip addr show to list)
```

**Persistent (via netplan, Ubuntu/Debian):**

1. Find your network interface:
   ```bash
   ip addr show
   # Look for your main interface, e.g. eth0, enp0s3, ens33
   ```

2. Edit `/etc/netplan/00-installer-config.yaml` (or your existing netplan file):
   ```yaml
   network:
     version: 2
     ethernets:
       eth0:
         dhcp4: true
         routes:
           - to: 224.0.0.0/4
             via: 127.0.0.1
   ```

3. Apply:
   ```bash
   sudo netplan apply
   ```

**Persistent (via ufw, if using firewall):**

Make sure multicast is not blocked:
```bash
sudo ufw allow in on eth0 from 224.0.0.1 to 224.0.0.1
sudo ufw allow out on eth0 to 224.0.0.1 from 224.0.0.1
```

**Persistent (via firewalld, Fedora/RHEL):**
```bash
sudo firewall-cmd --permanent --add-port=59485-59497/tcp
sudo firewall-cmd --permanent --add-port=59486-59497/udp
sudo firewall-cmd --reload
```

### Linux Release Tarball

After `./build-release.sh`, you'll have `dist/DG-LAN-X.Y.Z-Alpha-linux-x86_64.tar.gz` containing:

```
DG-LAN.Core                 ← Headless daemon
DG-LAN.GUI                  ← Qt5 GUI application
styles/                      ← UI stylesheets
languages/                   ← Translation files
dglan-core.service           ← systemd unit file
dglan.desktop                ← XDG desktop entry
install.sh                   ← Helper to install binaries + service
RELEASE-METADATA.txt         ← Build/distro/tooling provenance
```

**Extract and install:**
```bash
tar -xzf DG-LAN-*-linux-*.tar.gz
cd DG-LAN-*-linux-*
sudo ./install.sh             # installs to /usr/local by default
sudo ./install.sh /opt/dglan  # or specify a prefix
```

`install.sh` rewrites the shipped systemd unit and desktop entry to match the chosen install prefix, so `/opt/...` installs do not still point at `/usr/local/bin`.

**Or run directly (no install):**
```bash
tar -xzf DG-LAN-*-linux-*.tar.gz
cd DG-LAN-*-linux-*
./DG-LAN.Core -e &            # headless daemon in console mode
./DG-LAN.GUI &                 # Qt5 GUI
```

### Testing on Linux

Run the unified validation entrypoint:

```bash
python3 validate.py
```

**Expected output for a release-ready Linux candidate:**
- ✅ Python tests: 59/59 PASS (always attempted)
- ✅ Desktop/C++ validation: PASS (or BLOCKED in headless containers)
- ✅ `./build-release.sh -SkipPublish`: produces the tarball on the target distro/arch
- ✅ Tarball smoke: `DG-LAN.Core --version` works, GUI launches on a real desktop session, systemd install smoke passes if shipping as a service

See [TESTING.md](TESTING.md) for details.

### Known Limitations (Linux)

- ❌ **No auto-update**: Linux tarball releases don't auto-update (UpdateChecker looks for `.exe` assets only). Update manually.
- ❌ **Service integration**: Uses systemd (see `dglan-core.service` in the tarball)
- ❌ **Config paths**: Uses `~/.config/DGLan/` (XDG standard) instead of Windows `%APPDATA%` paths
- ⚠️ **IPv6**: Not yet implemented (same as Windows)
- ⚠️ **TLS**: Network isolation-based security; not yet HTTPS (same as Windows)

---

## macOS Build (Possible, Not Regularly Tested)

```bash
brew install qt@5 protobuf
export PATH="/usr/local/opt/qt@5/bin:$PATH"

cd application
qmake Core.pro -r && make -j$(nproc)
qmake GUI.pro -r && make -j$(nproc)
```

Follow [Linux Build](#linux-build-experimental-native-only) for similar gotchas and systemd replacement (macOS uses launchd).

---

## Dual-Release Strategy

DG-LAN supports two platform-specific release artifacts per version, built with a unified release command:

| Platform | Release Type | Built With | Auto-Update | Installer |
|----------|--------------|-----------|-------------|-----------|
| **Windows** | `.exe` installer | `./release.sh` (via `build-release.ps1`) | ✅ GitHub Releases API | Inno Setup 6 |
| **Linux** | `.tar.gz` tarball | `./release.sh` (via `build-release.sh`) | ❌ Manual | systemd unit + install.sh |

### Release Workflow

**Cross-Platform (Recommended):**

Use the unified release command on either platform:

```bash
./release.sh
# → Windows: Outputs DG-LAN-X.Y.Z-Setup.exe, uploads to GitHub Release
# → Linux:   Outputs dist/DG-LAN-X.Y.Z-Alpha-linux-x86_64.tar.gz, uploads to GitHub Release
```

**Platform-Native (Advanced):**

For direct control of the underlying scripts:

**Windows Release (on Windows machine):**
```powershell
.\build-release.ps1
# → Outputs: DG-LAN-X.Y.Z-Setup.exe
# → Uploads to GitHub Release
```

**Linux Release (on Linux machine):**
```bash
./build-release.sh
# → Outputs: dist/DG-LAN-X.Y.Z-Alpha-linux-x86_64.tar.gz
# → Uploads to GitHub Release (attaches alongside Windows .exe)
```

All release commands:
- Read/patch the same `Version.h`
- Auto-increment patch version
- Commit, tag, and push to GitHub
- Create/update the GitHub Release with appropriate assets

**Release rule:** only attach the Linux tarball to the shared GitHub Release after the native Linux build + smoke checklist passes on that distro/arch. Do not infer Raspberry Pi, Ubuntu, or RedHat support from a different Linux build.

### One Codebase, Two Artifacts

The C++ and Qt code is 100% platform-agnostic. No conditional compilation needed. Build machines just differ:

- **Windows**: MSYS2 MinGW64 toolchain
- **Linux**: System package manager Qt5 + GCC/Clang
- **macOS**: Homebrew Qt5 + Clang

All three produce functionally identical binaries (minus platform-specific paths/services).

---

## Running

### Core (headless daemon)
```bash
# Windows
application\Core\output\release\DG-LAN.Core.exe

# Linux / macOS
./application/Core/output/release/DG-LAN.Core
```

The Core also installs as a Windows service via the installer.

### GUI
```bash
# Windows
application\GUI\output\release\DG-LAN.GUI.exe

# Linux / macOS
./application/GUI/output/release/DG-LAN.GUI
```

---

## ZeroTier Setup

For cross-subnet use (e.g., connecting machines on different networks):

1. Install [ZeroTier](https://www.zerotier.com/download/) on all machines
2. Join the same ZeroTier network: `zerotier-cli join <network-id>`
3. Note the ZeroTier interface name:
   - **Windows**: Network adapter name in Device Manager, usually `ZeroTier One [...]`
   - **Linux**: `ip addr` — look for `zt...`
   - **macOS**: `ifconfig` — look for `ztXXXXXX`
4. In DG-LAN: **Settings → Network → Interface** → select the ZeroTier adapter
5. Set **Multicast TTL** to `1` for ZeroTier networks

> **ZeroTier network controller**: Ensure `multicastLimit > 0` and `enableBroadcast: true`
> in [ZeroTier Central](https://my.zerotier.com). If multicast is disabled, DG-LAN
> falls back to subnet scan + gossip discovery automatically.

---

## Firewall

Allow DG-LAN's port range through your OS firewall.

### Windows (PowerShell, run as Administrator)
```powershell
New-NetFirewallRule -DisplayName "DG-LAN UDP" -Direction Inbound -Protocol UDP -LocalPort 59486-59497 -Action Allow
New-NetFirewallRule -DisplayName "DG-LAN TCP" -Direction Inbound -Protocol TCP -LocalPort 59485-59497 -Action Allow
```

### Linux (ufw)
```bash
sudo ufw allow 59485:59497/tcp
sudo ufw allow 59486:59497/udp
```

---

## URL Scheme Registration (`dglan://`)

DG-LAN supports a custom `dglan://` URL scheme so that clicking a link on a web page queues a download in the running app.

```
dglan://download?peer=PEER_HEX&hash=ENTRY_HEX&size=BYTES&name=FILENAME&path=/
```

| Parameter | Description |
|-----------|-------------|
| `peer` | 56-char hex peer ID |
| `hash` | 56-char hex shared-entry ID |
| `size` | File size in bytes |
| `name` | Percent-encoded filename |
| `path` | Percent-encoded path (`/` for root) |

The `peer` and `hash` values come from DG-LAN's Browse / Search results.

### Windows

Run once (non-admin), replacing the path with your install location:

```bat
reg add "HKCU\Software\Classes\dglan"                          /ve /d "DG-LAN Protocol" /f
reg add "HKCU\Software\Classes\dglan"                          /v "URL Protocol" /d "" /f
reg add "HKCU\Software\Classes\dglan\DefaultIcon"              /ve /d "\"C:\Program Files\DG-LAN\D-LAN.GUI.exe\",0" /f
reg add "HKCU\Software\Classes\dglan\shell\open\command"       /ve /d "\"C:\Program Files\DG-LAN\D-LAN.GUI.exe\" \"%1\"" /f
```

Remove: `reg delete "HKCU\Software\Classes\dglan" /f`

### Linux

Create `~/.local/share/applications/dglan-handler.desktop`:

```ini
[Desktop Entry]
Name=DG-LAN URL handler
Exec=/path/to/D-LAN.GUI %u
MimeType=x-scheme-handler/dglan;
Type=Application
NoDisplay=true
```

```bash
chmod +x ~/.local/share/applications/dglan-handler.desktop
xdg-mime default dglan-handler.desktop x-scheme-handler/dglan
update-desktop-database ~/.local/share/applications
```

### macOS

Add to `D-LAN.GUI.app/Contents/Info.plist`:

```xml
<key>CFBundleURLTypes</key>
<array>
    <dict>
        <key>CFBundleURLName</key>
        <string>DG-LAN Protocol</string>
        <key>CFBundleURLSchemes</key>
        <array>
            <string>dglan</string>
        </array>
    </dict>
</array>
```

Then re-register:
```bash
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister \
    -f /Applications/D-LAN.GUI.app
```

---

## Generating `dglan://` Links from a Web Page

```html
<a href="dglan://download?peer=0011aabb...&hash=ccdd1122...&size=104857600&name=movie.mkv&path=/">
    Download with DG-LAN
</a>
```

```javascript
function dglanLink(peer, hash, size, name, path) {
    const params = new URLSearchParams({
        peer, hash,
        size: String(size),
        name,
        path: path || '/'
    });
    return 'dglan://download?' + params.toString();
}
```

See also: [dglan-api/](dglan-api/) — a Python HTTP server that generates these links from live Core data.
