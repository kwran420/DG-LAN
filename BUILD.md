# DG-LAN Build Instructions

DG-LAN is a fork of [D-LAN](https://github.com/Ummon/D-LAN) with ZeroTier support, gossip-based peer discovery, and core seeder mode.

---

## Dependencies

### All Platforms
- **Qt 5.15.x** (with Qt Network and Qt Widgets modules)
- **Protocol Buffers v3** (`protoc` compiler + C++ runtime library)

### Windows
1. Install [Qt 5.15.x](https://www.qt.io/download) with the **MSVC 2019 64-bit** kit
2. Install [Visual Studio 2019 Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) (C++ workload)
3. Install [protobuf](https://github.com/protocolbuffers/protobuf/releases) — download the Windows binary release, add `bin/` to your `PATH`
4. Verify: open a **Developer Command Prompt for VS 2019** and run:
   ```
   qmake --version
   protoc --version
   cl
   ```

### Linux (Debian/Ubuntu)
```bash
sudo apt-get update
sudo apt-get install -y qt5-default qtbase5-dev qttools5-dev \
    libprotobuf-dev protobuf-compiler build-essential
```

### macOS
```bash
brew install qt@5 protobuf
export PATH="/usr/local/opt/qt@5/bin:$PATH"
```

---

## Build

### Linux / macOS
```bash
git clone https://github.com/YOUR_USERNAME/DG-LAN.git
cd DG-LAN/application
qmake D-LAN.pro
make -j$(nproc)
```

### Windows (Developer Command Prompt for VS 2019)
```bat
git clone https://github.com/YOUR_USERNAME/DG-LAN.git
cd DG-LAN\application
qmake D-LAN.pro
nmake
```

Or use the provided script:
```bat
build.bat
```

---

## Running

### Core (daemon / headless)
```bash
# Linux
./Core/Core

# Windows
Core\Core.exe
```

### GUI Client
```bash
# Linux
./GUI/GUI

# Windows
GUI\GUI.exe
```

---

## ZeroTier Setup

1. Install [ZeroTier](https://www.zerotier.com/download/) on all machines
2. Join the same ZeroTier network: `zerotier-cli join <network-id>`
3. Note your ZeroTier interface name:
   - **Linux**: `ip addr` — look for `zt...` interface
   - **Windows**: Network Adapter name in Device Manager, usually `ZeroTier One [...]`
   - **macOS**: `ifconfig` — look for `ztXXXXXX` interface
4. In DG-LAN Settings → Network → **Interface**: select the ZeroTier interface
5. Set **Multicast TTL** to `1` for ZeroTier networks

> **ZeroTier network controller requirements**: For multicast/broadcast to work,
> ensure your ZeroTier network has `multicastLimit > 0` and `enableBroadcast: true`
> configured in the [ZeroTier Central](https://my.zerotier.com) network settings.
> If not, DG-LAN will automatically fall back to subnet scan + gossip discovery.

---

## Core Seeder Setup

A Core Seeder is just a regular DG-LAN instance that runs 24/7 and holds the files.

1. Enable: Settings → Network → **"This machine is a Core Seeder"**
2. Share your files as normal via the GUI
3. Other peers: Settings → Network → Core Seeders → **Add** the seeder's ZeroTier IP

Core Seeders are probed first at startup, before the full subnet scan begins.

---

## Ports Used

| Port | Protocol | Purpose |
|------|----------|---------|
| 59486 | UDP | Multicast discovery |
| 59487 | UDP + TCP | Unicast peer communication (increments if busy) |
| 59485 | TCP | Remote GUI control |

Ensure these are allowed through your OS firewall on the ZeroTier interface.

### Windows Firewall (PowerShell, run as Administrator)
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

DG-LAN supports a custom URL scheme so that clicking a `dglan://` link in a browser automatically adds the file to the download queue of your already-running DG-LAN instance.

The URL format is:
```
dglan://download?peer=PEER_HEX&hash=ENTRY_HEX&size=BYTES&name=FILENAME&path=/relative/path
```

| Parameter | Description |
|-----------|-------------|
| `peer`    | 56-character hex ID of the peer that holds the file |
| `hash`    | 56-character hex ID of the shared entry |
| `size`    | File size in bytes |
| `name`    | Percent-encoded filename |
| `path`    | Percent-encoded path inside the shared entry (use `/` for root) |

The `peer` and `hash` values must come from DG-LAN's own Browse / Search results — they are not human-guessable.

### Register on Windows

Run once from a normal (non-admin) Command Prompt, replacing the path with your actual install path:

```bat
reg add "HKCU\Software\Classes\dglan"                          /ve /d "DG-LAN Protocol" /f
reg add "HKCU\Software\Classes\dglan"                          /v "URL Protocol" /d "" /f
reg add "HKCU\Software\Classes\dglan\DefaultIcon"              /ve /d "\"C:\Program Files\DG-LAN\D-LAN.GUI.exe\",0" /f
reg add "HKCU\Software\Classes\dglan\shell\open\command"       /ve /d "\"C:\Program Files\DG-LAN\D-LAN.GUI.exe\" \"%1\"" /f
```

To remove the registration:
```bat
reg delete "HKCU\Software\Classes\dglan" /f
```

### Register on Linux

Create `~/.local/share/applications/dglan-handler.desktop`:

```ini
[Desktop Entry]
Name=DG-LAN URL handler
Exec=/path/to/D-LAN.GUI %u
MimeType=x-scheme-handler/dglan;
Type=Application
NoDisplay=true
```

Then register and update the MIME database:
```bash
chmod +x ~/.local/share/applications/dglan-handler.desktop
xdg-mime default dglan-handler.desktop x-scheme-handler/dglan
update-desktop-database ~/.local/share/applications
```

Verify:
```bash
xdg-mime query default x-scheme-handler/dglan
# should print: dglan-handler.desktop
```

### Register on macOS

Add the following key to `D-LAN.GUI.app/Contents/Info.plist` (edit it or add it to your Xcode target's Info tab):

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

After editing the plist, re-register with Launch Services:
```bash
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister \
    -f /Applications/D-LAN.GUI.app
```

---

## Generating `dglan://` Links from a Web Page

Any web page can link directly into DG-LAN.  The `peer` and `hash` values come from DG-LAN itself (e.g. exported from a Browse result).

```html
<!-- Static link -->
<a href="dglan://download?peer=0011aabb...&hash=ccdd1122...&size=104857600&name=movie.mkv&path=/">
    Download with DG-LAN
</a>

<!-- JavaScript helper -->
<script>
function dglanLink(peer, hash, size, name, path) {
    const params = new URLSearchParams({
        peer, hash,
        size: String(size),
        name,
        path: path || '/'
    });
    return 'dglan://download?' + params.toString();
}

document.getElementById('dl-btn').href =
    dglanLink(
        '0011aabbccddeeff...',   // 56-char peer hex
        'aabbccddeeff0011...',   // 56-char entry hash hex
        104857600,               // bytes
        'movie.mkv',
        '/'
    );
</script>
```
