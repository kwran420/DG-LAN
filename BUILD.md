# DG-LAN — Build Instructions

DG-LAN is built with **MSYS2 MinGW64** on Windows (the primary platform). Linux and macOS builds are possible but untested.

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

### One-Command Build

From PowerShell in the repo root:

```powershell
.\build-release.ps1 -SkipPublish   # build only — no git push
```

This:
- Patches `Version.h` with the build timestamp and git hash
- Auto-increments the patch version (e.g. 1.2.85 → 1.2.86)
- Builds Core and GUI via MSYS2 MinGW64
- Builds the Inno Setup installer

**Output:**
- `application/Core/output/release/DG-LAN.Core.exe`
- `application/GUI/output/release/DG-LAN.GUI.exe`
- `application/Setups/Windows/Installations/DG-LAN-<version>-Setup.exe`

### Build + Publish (Release)

```powershell
.\build-release.ps1               # default: build + commit + tag + push + GitHub Release
```

This does everything above, plus:
- Commits `Version.h`, tags with `v<version>`, pushes to origin
- Creates a GitHub Release and uploads the installer `.exe`

### Build Script Flags

| Flag | Effect |
|------|--------|
| *(no flags)* | Build + publish to GitHub Releases |
| `-SkipPublish` | Build locally, no git push or release |
| `-SkipBuild` | Skip compilation, rebuild installer only |
| `-Version 2.0.0` | Override the version number |

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

```bash
sudo apt-get update
sudo apt-get install -y qt5-default qtbase5-dev qttools5-dev \
    libprotobuf-dev protobuf-compiler build-essential

cd application
qmake Core.pro -r && make -j$(nproc)
qmake GUI.pro -r && make -j$(nproc)
```

---

## macOS Build (Experimental)

```bash
brew install qt@5 protobuf
export PATH="/usr/local/opt/qt@5/bin:$PATH"

cd application
qmake Core.pro -r && make -j$(nproc)
qmake GUI.pro -r && make -j$(nproc)
```

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
