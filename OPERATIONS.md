# DG-LAN — Operations & Deployment Guide

Runbook for system administrators, operations teams, and users deploying DG-LAN in production or lab environments.

**Target audience:** DevOps, sysadmins, lab operators, advanced end-users running 24/7 instances.

**Platform notes:** This guide covers **Windows (primary)** and **Linux (experimental, native per distro/arch)**. See [Platform-Specific Setup](#platform-specific-setup) for distro-specific configuration.

---

## Table of Contents

1. [Platform-Specific Setup](#platform-specific-setup)
2. [Running as Windows Service](#running-as-windows-service)
3. [Running on Linux](#running-on-linux)
4. [Configuration & Settings](#configuration--settings)
5. [Logging & Monitoring](#logging--monitoring)
6. [Troubleshooting](#troubleshooting)
7. [Performance Tuning](#performance-tuning)
8. [Backup & Recovery](#backup--recovery)
9. [Security Considerations](#security-considerations)
10. [Known Limitations](#known-limitations)

---

## Platform-Specific Setup

### Release Artifacts & Validation

DG-LAN uses [`release.sh`](release.sh) as the canonical Windows/Linux release entrypoint. It dispatches to [`build-release.ps1`](build-release.ps1) on Windows and [`build-release.sh`](build-release.sh) on Linux.

| Platform | Release Artifact | Validation | Installation |
|----------|------------------|-----------|--------------|
| **Windows (x86_64)** | `DG-LAN-<version>-Setup.exe` (Inno Setup installer) | Binary signed, EXE contains both Core and GUI installers | Run .exe; auto-installs to `C:\Program Files\DG-LAN\` + configures Windows Service |
| **Linux (x86_64, ARM64, ARMv7)** | `dist/DG-LAN-<version>-<tag>-linux-{arch}.tar.gz` (native tarball per arch) | Unpacked, binaries tested on build platform (smoke: `DG-LAN.Core --version`, wrapper/package smoke) | Extract tarball, run `install.sh` or install manually |

**Important:** Linux tarballs are **platform and architecture specific**. A tarball built on Ubuntu 20.04 x86_64 may not run on Fedora x86_64 or Raspberry Pi ARM64. Native builds on the target platform are strongly recommended for production.

### Release Workflow

Both release scripts follow this pattern:

1. **Read Version.h** (single source of truth)
2. **Patch Version.h** with build timestamp and git hash (local Linux `-SkipPublish` builds restore it on exit)
3. **Compile** Core and GUI
4. **Package** native artifact (`.exe` on Windows, `.tar.gz` on Linux)
5. **Test** artifact (Windows: installer smoke; Linux: binary execution smoke)
6. **Optional publish**: Git commit + tag + GitHub Release upload (unless `-SkipPublish`)

For detailed build instructions, see [BUILD.md](BUILD.md).

---

## Running as Windows Service

### Automated Installation (via Installer)

The DG-LAN installer (`.exe`) automatically installs Core as a Windows service named `DG-LAN Core`. The service is set to **auto-start** on system boot.

**To verify the service is installed:**

```powershell
Get-Service "DG-LAN Core" -ErrorAction SilentlyContinue
```

Expected output:
```
Status   Name               DisplayName
------   ----               -----------
Running  DG-LAN Core        DG-LAN Core
```

### Manual Service Management

#### Start the service:
```powershell
net start "DG-LAN Core"
# or
Start-Service -Name "DG-LAN Core"
```

#### Stop the service:
```powershell
net stop "DG-LAN Core"
# or
Stop-Service -Name "DG-LAN Core"
```

#### Restart:
```powershell
Restart-Service -Name "DG-LAN Core"
```

#### Query service status:
```powershell
Get-Service "DG-LAN Core"
sc query "DG-LAN Core"
```

#### View service logs in Event Viewer:

1. Open **Event Viewer** (`eventvwr.msc`)
2. Navigate to **Windows Logs → Application**
3. Filter by Source: `DG-LAN Core`

### Manual Service Installation (from Command Line)

If the service is not installed, you can manually install it:

```powershell
# Run as Administrator
cd "C:\Program Files\DG-LAN"
.\DG-LAN.Core.exe -i

# With specific account (e.g., NETWORK SERVICE)
.\DG-LAN.Core.exe -i "NT AUTHORITY\NETWORK SERVICE" ""
```

### Manual Service Uninstall

```powershell
# Run as Administrator
cd "C:\Program Files\DG-LAN"
.\DG-LAN.Core.exe -u
```

### Running Core as a Standalone Application (Not as Service)

For testing or debugging, run Core without the service wrapper:

```powershell
cd "C:\Program Files\DG-LAN"
.\DG-LAN.Core.exe -e
```

The `-e` flag launches Core as a regular application (useful for development).

---

## Running on Linux

### Installation from Tarball

1. **Extract the tarball:**
   ```bash
   tar xzf DG-LAN-1.2.x-Alpha-linux-x86_64.tar.gz
   cd DG-LAN-1.2.x-Alpha-linux-x86_64/
   ```

2. **Review the contents:**
   ```bash
   ls -la
   # Expected: DG-LAN.Core, DG-LAN.GUI, systemd service file, install script
   ```

3. **Install (choose one):**

   **Option A: Automated setup (recommended for systemd systems):**
   ```bash
   sudo ./install.sh              # defaults to /usr/local
   sudo ./install.sh /opt/dglan   # optional custom prefix
   ```
   This installs binaries under `<prefix>/bin`, shared assets under `<prefix>/share/dglan`, rewrites the shipped systemd unit / desktop entry to that prefix, and reloads systemd if available.

   **Option B: Manual installation:**
   ```bash
   sudo install -Dm755 DG-LAN.Core /usr/local/bin/DG-LAN.Core
   sudo install -Dm755 DG-LAN.GUI /usr/local/bin/DG-LAN.GUI
   sudo mkdir -p /usr/local/share/dglan
   sudo cp -R styles languages /usr/local/share/dglan/
   ```

### Running as systemd Service (Recommended)

The packaged Linux service is a system-level `systemd` unit named `dglan-core.service`.

**Before enabling it, create the dedicated account expected by the unit:**
```bash
sudo useradd -r -s /usr/sbin/nologin dglan
```

**If installed via `install.sh`:**
```bash
sudo systemctl start dglan-core
sudo systemctl status dglan-core
sudo systemctl enable dglan-core
```

**Check logs:**
```bash
sudo journalctl -u dglan-core -f
sudo journalctl -u dglan-core -n 50
```

### Running Standalone (Not as Service)

For testing or headless environments, run from the extracted tarball or installed prefix:

```bash
/usr/local/bin/DG-LAN.Core -e &
# or with output redirected:
/usr/local/bin/DG-LAN.Core -e > ~/dg-lan-core.log 2>&1 &
```

### GUI on Linux (Display Required)

The GUI requires a display (X11 or Wayland). On a remote machine, use X11 forwarding or VNC.

**Local GUI launch:**
```bash
/usr/local/bin/DG-LAN.GUI
```

**Remote GUI (via X11 forwarding):**
```bash
ssh -X user@remote-machine
/usr/local/bin/DG-LAN.GUI
```

### Linux-Specific Configuration

**Config directory:**
```bash
~/.config/DGLan/        # XDG_CONFIG_HOME (settings.xml, protocol files)
~/.local/share/DGLan/   # XDG_DATA_HOME (cache, logs, queue)
```

**Firewall (UFW example):**
```bash
sudo ufw allow 59485/tcp      # GUI ↔ Core
sudo ufw allow 59486/udp      # Peer discovery (multicast)
sudo ufw allow 59487/udp      # Peer data
```

**Multicast routing (if peers on different subnets):**

If using ZeroTier or bridged networks, ensure multicast TTL is set in settings:
```bash
# Edit ~/.config/DGLan/settings.xml or via GUI:
# <multicast_ttl>64</multicast_ttl>   # for ZeroTier or multi-hop networks
```

See [BUILD.md — Linux Multicast Routing](BUILD.md#linux-multicast-routing) for detailed netplan/firewall setup.

---

## Configuration & Settings

### Default Installation Paths

#### Windows

| Component | Path |
|-----------|------|
| Program files | `C:\Program Files\DG-LAN\` |
| Roaming data (settings) | `%APPDATA%\DGLan\` |
| Local data (cache, logs, queue) | `%LOCALAPPDATA%\DGLan\` |

#### Linux

| Component | Path |
|-----------|------|
| Program files | `/usr/local/bin/` (default `install.sh` prefix) |
| Shared assets | `/usr/local/share/dglan/` (default `install.sh` prefix) |
| Settings | `~/.config/DGLan/` (XDG_CONFIG_HOME) |
| Data (cache, logs, queue) | `~/.local/share/DGLan/` (XDG_DATA_HOME) |

### Settings Files

#### Main Settings

**Location:** `%APPDATA%\DGLan\settings.xml` (or `core_settings.bin` — protobuf binary format)

**Key settings:**

| Setting | Default | Meaning |
|---------|---------|---------|
| `nick` | `<hostname>` | Peer display name |
| `peerID` | Random | Unique peer identifier (56-char hex) |
| `client_mode` | `false` | `true` = client (selective re-hosting); `false` = master |
| `shared_folders` | *(empty)* | List of directories to share |
| `interface` | `0.0.0.0` | Network interface (auto-detect or manual ZeroTier) |
| `port` | `59487` | Base port (auto-increments if busy) |
| `multicast_ttl` | `1` | Multicast TTL (use `1` for LAN, higher for ZeroTier) |
| `listen_port_http` | `59480` | HTTP server port |
| `gui_listening_port` | `59485` | GUI ↔ Core TCP port |

#### File Cache

**Location:** `%LOCALAPPDATA%\DGLan\file_cache.bin`

- Binary protobuf file, persists directory index across restarts
- Written every ~60 seconds during normal operation
- Deleted and regenerated on startup if corrupted
- On first run: empty; full initial scan takes seconds to minutes (depends on folder size)

#### Download Queue

**Location:** `%LOCALAPPDATA%\DGLan\` (queue state stored in QSettings, not as separate file)

- Persisted via Qt QSettings
- Lost on app crash or unclean shutdown

#### Log Files

**Location:** `%LOCALAPPDATA%\DGLan\Logs\`

- Log files are **NOT** rotated automatically; grow unbounded
- See [Logging & Monitoring](#logging--monitoring) for manual cleanup

### Editing Settings Programmatically

**Via GUI:** Settings → Preferences dialog (covers most common options)

**Via command line (Core only):**

```powershell
# Set language
.\DG-LAN.Core.exe --lang de

# Set password
.\DG-LAN.Core.exe --pass "mypassword"

# Remove password
.\DG-LAN.Core.exe --rmpass

# Reset all settings to defaults (keeps nick + peerID)
.\DG-LAN.Core.exe --reset-settings

# Query version
.\DG-LAN.Core.exe --version
```

### Firewall Configuration

DG-LAN requires UDP and TCP ports `59485–59497` to be open. See [BUILD.md — Firewall](BUILD.md#firewall) for automated PowerShell rules.

**Manual configuration:**

```powershell
# Windows Defender Firewall with Advanced Security
netsh advfirewall firewall add rule name="DG-LAN UDP" dir=in action=allow protocol=UDP localport=59486-59497
netsh advfirewall firewall add rule name="DG-LAN TCP" dir=in action=allow protocol=TCP localport=59485-59497
```

---

## Logging & Monitoring

### Log Levels

DG-LAN uses the following log levels (in code: `L_DEBUG`, `L_INFO`, `L_WARN`, `L_ERROR`, `L_CRIT`, `L_USER`):

| Level | Purpose | Example |
|-------|---------|---------|
| `DEBUG` | Detailed diagnostic info | Hash calculation progress, socket events |
| `INFO` | Normal operations | Peer joined, file indexed, download started |
| `WARN` | Recoverable issues | Peer connection timeout, slow scan |
| `ERROR` | Errors (app continues) | File not found, hash mismatch, network error |
| `CRIT` | Critical errors (near crash) | Out of memory, corrupted settings |
| `USER` | User-visible messages | "Shutdown", "Update available" |

### Accessing Logs

#### Real-time (from Running App)

- **GUI:** View → Log dock (bottom panel by default)
- **Core (Windows standalone):** Logs printed to console
- **Core (Linux systemd):** `journalctl --user -u dg-lan-core -f` (if service installed)
- **Core (Linux standalone):** Output to stdout or redirection target

#### On-Disk Log Files

**Windows:**
- **Path:** `%LOCALAPPDATA%\DGLan\Logs\`
- **Format:** Plain text, timestamped
- **Rotation:** Not automatic — files grow indefinitely

**Linux:**
- **Path:** `~/.local/share/DGLan/Logs/` or `~/.local/share/DGLan/` (XDG standard)
- **Format:** Plain text, timestamped
- **Rotation:** Not automatic — files grow indefinitely
- **Systemd integration:** Logs also written to journalctl (query via `journalctl --user -u dg-lan-core`)

**Manual log cleanup (on server with 24/7 uptime):**

*Windows:*
```powershell
# Archive old logs (keep last 30 days)
$cutoffDate = (Get-Date).AddDays(-30)
Get-ChildItem "C:\Users\<user>\AppData\Local\DGLan\Logs" -Filter "*.log" |
    Where-Object { $_.LastWriteTime -lt $cutoffDate } |
    Move-Item -Destination "C:\Logs\DGLan\Archive\"
```

*Linux:*
```bash
# Archive old logs (keep last 30 days)
find ~/.local/share/DGLan/Logs -name "*.log" -mtime +30 -exec gzip {} \; -exec mv {}.gz archive/ \;
# Or delete:
find ~/.local/share/DGLan/Logs -name "*.log" -mtime +30 -delete
```

### Monitoring for Health

#### Key Metrics

| Metric | Healthy | Warning | Critical |
|--------|---------|---------|----------|
| Memory usage (Core) | <200 MB | 200–500 MB | >500 MB |
| Peer count | >1 | 1 | 0 (isolated) |
| Indexed files | Stable | Growing slowly | Growing rapidly |
| HTTP server response time | <100 ms | 100–500 ms | >1 s |
| Pending downloads | Decreasing | Stable | Growing indefinitely |

#### Simple Health Check

```powershell
# Check if Core service is running
(Get-Service "DG-LAN Core").Status -eq "Running"

# Check if TCP :59480 (HTTP server) is listening
(netstat -ano | Select-String ":59480" | Measure-Object).Count -gt 0

# Check if multicast probe is working (requires tcpdump/Wireshark)
# tcpdump -i <interface> "udp port 59486"
```

#### Monitoring with Event Viewer

1. Open **Event Viewer** → **Windows Logs → Application**
2. Right-click → **Filter Current Log**
3. Set Source to `DG-LAN Core`
4. Click OK

#### Auto-Restart on Crash (Advanced)

To automatically restart Core if it crashes, use **Task Scheduler**:

1. **Create task:** "DG-LAN Core Auto-Restart"
2. **Trigger:** On system startup (or custom)
3. **Action:** Run `net start "DG-LAN Core"`
4. **Set user:** SYSTEM (or account running Core)

---

## Troubleshooting

### Platform-Specific Diagnostic Checklist

**Windows:**
```powershell
Get-Service "DG-LAN Core"
netstat -ano | Select-String "59486"
Get-Content "$env:LOCALAPPDATA\DGLan\Logs\*.log" -Tail 50
```

**Linux (systemd service):**
```bash
sudo systemctl status dglan-core
netstat -tlnp | grep -E "(59485|59486|59487)"
sudo journalctl -u dglan-core -n 50
```

**Linux (standalone):**
```bash
ps aux | grep DG-LAN.Core
netstat -tlnp | grep -E "(59485|59486|59487)"
# Check log file directly:
tail -50 ~/.local/share/DGLan/Logs/*.log
```

---

### Peer Discovery Not Working

**Symptom:** Core is running but no peers appear in the peer list.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| Firewall blocking UDP :59486 | Add firewall rule (Windows: [Firewall Configuration](#firewall-configuration)); Linux: `sudo ufw allow 59486/udp` |
| Wrong network interface | Settings → Network → Interface (manually select correct NIC) |
| Multicast disabled | Try manual broadcast/subnet scan (auto-fallback should work) |
| Different subnets | Use ZeroTier (see [BUILD.md → ZeroTier Setup](BUILD.md#zerotier-setup)) |
| Core service not running | Windows: `net start "DG-LAN Core"`; Linux: `sudo systemctl start dglan-core` |
| Multicast TTL too low | Set to `1` for LAN; higher for ZeroTier |
| Linux: SELinux blocking multicast | `getenforce` to check; may need custom policy if enforcing |

**Diagnostic steps (Linux):**

```bash
# 1. Verify Core is running
sudo systemctl is-active dglan-core  # or: ps aux | grep DG-LAN.Core

# 2. Verify ports are listening
netstat -tlnp | grep DG-LAN

# 3. Check firewall (UFW example)
sudo ufw status | grep 594

# 4. Test multicast on local interface
# (requires multicast tools; install: apt-get install mping)
# mping -I <interface>

# 5. Check logs for discovery errors
sudo journalctl -u dglan-core -n 100 | grep -i discovery
```

### Slow File Indexing

**Symptom:** Core is scanning a folder; progress is slow or stuck.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| Very large folder (>100K files) | Indexing is expected to take several minutes; check task manager (CPU/disk usage should be >10%) |
| Slow disk (network share) | Avoid indexing network shares; cache data locally first |
| Many small files (>1M files) | Memory usage may spike; add RAM if available |
| File permissions issues | Ensure Core process has read permission on all shared folders |

**Mitigation:**

- Don't index the entire C: drive; select specific folders
- Exclude system folders (`Windows`, `Program Files`, temp folders)
- Consider adding folders incrementally rather than all at once

### Peer Connection Drops Frequently

**Symptom:** Peers connect, then disconnect within seconds; constant reconnection.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| Network instability | Check router logs; consider wired instead of WiFi |
| Firewall rules incomplete | Verify both TCP and UDP rules allow port range 59485–59497 |
| NAT/router port forwarding misconfigured | UDP traffic may be blocked; use ZeroTier for cross-NAT |
| Peer running old protocol version | Peers must have compatible versions (v1.2.x supports v1.2.y) |

**Diagnostic:**

```powershell
# Monitor peer connections in real-time
Get-Process DG-LAN.Core | ForEach-Object {
    netstat -ano | Select-String $_.ID | Select-String "59"
}
```

### GUI Cannot Connect to Remote Core

**Symptom:** GUI shows "Cannot connect to Core" or times out.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| GUI pointing to wrong IP | Settings → Core Address (should be `localhost` or machine IP) |
| TCP :59485 blocked by firewall | Add rule: port 59485 TCP inbound |
| Core not listening on :59485 | Check `RemoteControlManager` in Core logs; restart Core |
| Network cable unplugged | (If Core and GUI on different machines) Verify connectivity |

**Diagnostic:**

```powershell
# From GUI machine, test Core connectivity
Test-NetConnection -ComputerName <core-ip> -Port 59485

# Or use telnet
telnet <core-ip> 59485
```

### Download Stuck at 99%

**Symptom:** File download reaches 99% but never completes.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| Source peer disconnected unexpectedly | Resume download; if peer is back online, it will continue |
| Partial hash mismatch | Delete and re-download (force re-compute hash) |
| Disk full | Free up disk space; move download target folder to different drive |
| File corrupted on source | Source peer should re-index; delete source file and try again |

**Mitigation:**

- Ensure sufficient disk space (check `%LOCALAPPDATA%\DGLan\` target folder)
- Check download source peer is still online

### HTTP Server Returning 404 for Existing Files

**Symptom:** Accessing `http://machine:59480/path/to/file.mkv` returns 404 even though file exists.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| File path contains special characters | URL-encode path (e.g., spaces → `%20`) |
| File not yet indexed | Wait for FileManager scan to complete (check logs) |
| File in client mode but not downloaded | Client mode only shares downloaded files; master mode shares all |
| HTTP server not listening | Check settings: should be on port 59480 by default |

**Diagnostic:**

```powershell
# Test HTTP server is listening
(netstat -ano | Select-String ":59480" | Measure-Object).Count -gt 0

# Fetch root listing (should return JSON or HTML)
curl http://localhost:59480/
```

### High Memory Usage

**Symptom:** `DG-LAN.Core.exe` process grows to >500 MB RAM.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| Cache contains millions of files | This is expected; ~1 MB per 50K files. No pagination implemented yet. |
| Memory leak in download transfers | (Rare) Restart Core; if memory slowly grows during idle, report bug |
| WordIndex (search) building | Initial scan indexes all file names; memory spike is temporary |
| Gossip candidate list not expiring | Check: candidates should be capped at 50 entries with 30-min TTL |

**Workaround:**

- Restart Core periodically (e.g., nightly) if memory grows unbounded
- Reduce shared folder scope; don't index all files
- Monitor with Task Manager; if memory stabilizes after initial scan, it's normal

---

## Performance Tuning

### Network Interface Selection

If machine has multiple NICs (Ethernet, WiFi, VPN, ZeroTier), explicitly select one:

**Via GUI:**
- Settings → Preferences → Network → Interface

**Via settings file:**
- Windows: Edit `%APPDATA%\DGLan\settings.xml`
- Linux: Edit `~/.config/DGLan/settings.xml`

**Best practice:**
- Prefer wired (Ethernet) for stable peer discovery
- If using ZeroTier, select the ZeroTier virtual interface explicitly
- Linux: Use `ip link show` to list interfaces, then set `<interface>eth0</interface>` in settings.xml

### Multicast TTL Tuning

**For LAN (same subnet):**
```xml
<multicast_ttl>1</multicast_ttl>
```

**For ZeroTier (virtual network):**
```xml
<multicast_ttl>64</multicast_ttl>
```

**For multi-hop WAN (not recommended):**
```xml
<multicast_ttl>255</multicast_ttl>
```

### Download Priority Scheduling

Peers can be assigned priority in GUI: **High**, **Normal**, **Low**

- **High:** Download from this peer first (useful for fast, stable peers)
- **Normal:** Default priority
- **Low:** Use only if other peers fail

**Via GUI:** Right-click peer → Set Priority

### HTTP Server Tuning

The HTTP server has no configurable performance parameters. To limit bandwidth:

- Use OS-level QoS (Quality of Service) on the router
- Limit TCP window size via `netsh int tcp` (Windows)

```powershell
# Example: limit TCP window to 65KB (reduces peak throughput)
netsh int tcp set global autotuninglevel=disabled
netsh int tcp set global maxdataretransmissions=32
```

### Shared Folder Best Practices

1. **Use NTFS** (not FAT32) for better metadata caching
2. **Avoid network shares** — copy files to local disk first
3. **Index only what you need** — don't add entire C: drive
4. **Exclude system folders** — `Windows`, `Program Files`, temp, recycle bin
5. **Use SSD if available** — scanning HDD is slow

---

## Backup & Recovery

### What to Backup

| Item | Location (Windows) | Location (Linux) | Importance | Frequency |
|------|------------|------------------|-----------|-----------|
| Settings | `%APPDATA%\DGLan\settings.xml` | `~/.config/DGLan/settings.xml` | HIGH | Daily |
| File cache | `%LOCALAPPDATA%\DGLan\file_cache.bin` | `~/.local/share/DGLan/file_cache.bin` | MEDIUM | Daily (auto-rebuilt on loss) |
| Shared files | User-defined folders | User-defined folders | HIGH | Per deployment policy |

### Backup Strategy

**Windows — Minimal (settings only):**
```powershell
$backup = "C:\Backups\DGLan_$(Get-Date -Format 'yyyyMMdd').zip"
Compress-Archive -Path "$env:APPDATA\DGLan\settings.xml" -DestinationPath $backup
```

**Linux — Minimal (settings only):**
```bash
backup_dir=~/backups/DGLan
mkdir -p "$backup_dir"
cp ~/.config/DGLan/settings.xml "$backup_dir/settings_$(date +%Y%m%d).xml"
```

**Linux — Full (settings + cache + logs):**
```bash
backup_archive=~/backups/DGLan_$(date +%Y%m%d).tar.gz
tar czf "$backup_archive" ~/.config/DGLan ~/.local/share/DGLan
```

### Recovery

**Windows:**

1. **Stop Core service:**
   ```powershell
   net stop "DG-LAN Core"
   ```

2. **Restore files:**
   ```powershell
   Expand-Archive -Path "C:\Backups\DGLan_<date>.zip" -DestinationPath "$env:APPDATA"
   ```

3. **Start Core service:**
   ```powershell
   net start "DG-LAN Core"
   ```

**Linux (systemd service):**

1. **Stop Core service:**
   ```bash
   sudo systemctl stop dglan-core
   ```

2. **Restore files:**
   ```bash
   tar xzf ~/backups/DGLan_<date>.tar.gz -C ~/
   # or for settings only:
   cp ~/backups/settings_<date>.xml ~/.config/DGLan/settings.xml
   ```

3. **Start Core service:**
   ```bash
   sudo systemctl start dglan-core
   ```

### Cache Rebuild

If `file_cache.bin` is corrupted or lost:

1. Delete or rename the file
2. Restart Core — automatic full scan and rebuild (may take several minutes)

**Windows:**
```powershell
Remove-Item "$env:LOCALAPPDATA\DGLan\file_cache.bin"
net stop "DG-LAN Core"
net start "DG-LAN Core"
```

**Linux:**
```bash
rm ~/.local/share/DGLan/file_cache.bin
sudo systemctl restart dglan-core
```

---

## Security Considerations

### Master Password

**Purpose:** Restrict which peers can join the network (basic access control).

**Set password:**
```powershell
.\DG-LAN.Core.exe --pass "MySecurePassword123"
```

**Remove password:**
```powershell
.\DG-LAN.Core.exe --rmpass
```

**Behavior:**
- Peers must supply the password when connecting
- Password is transmitted over TCP, NOT encrypted (consider SSL/TLS upgrade for future)
- Password stored in `settings.xml` (hash, not plaintext)

**Security notes:**
- **DO NOT use the same password across multiple DG-LAN deployments**
- **Consider the network trustworthy** — password is not a substitute for network isolation
- Recommended for lab/office environments; not sufficient for untrusted networks

### Firewall Isolation

- Restrict DG-LAN ports (59485–59497) to known peers only (if possible)
- Use Windows Firewall profiles: Home, Work, Public

```powershell
# Example: allow DG-LAN only from 192.168.1.0/24 subnet
netsh advfirewall firewall add rule name="DG-LAN (Trusted LAN)" `
    dir=in action=allow protocol=UDP localport=59486-59497 `
    remoteip="192.168.1.0/24"
```

### HTTP Server Access

- **No authentication** on built-in HTTP server (port 59480)
- Any machine on the network can browse and download files
- For restricted access, use Python API bridge (dglan-api) with CORS + optional auth

### Shared Folder Permissions

- Ensure shared folders are not world-readable (if machine is on untrusted network)
- DG-LAN respects NTFS permissions; respect ACLs

### Updates & Patching

- DG-LAN auto-checks GitHub Releases on launch
- Enable "Check for updates on launch" in Settings
- Manual update available: Help → Check for Updates

---

## Known Limitations

| Limitation | Impact | Workaround | Target Fix | Platforms |
|-----------|--------|-----------|-----------|-----------|
| No automatic log rotation | Logs grow unbounded; may fill disk | Manual cleanup (see [Logging](#logging--monitoring)) | v1.3 | All |
| No IPv6 support | IPv6-only networks not supported | Use dual-stack or IPv4-only network | v2.0 | All |
| Selective rehosting in client mode | Fewer file sources, slower downloads | Master mode has better performance | By design | All |
| No bandwidth limiting | Fast peer can saturate network | Use OS QoS or limit TCP window | v1.4 | All |
| Single password (no user roles) | Coarse access control | Rely on firewall + network isolation | v2.0 | All |
| No TLS/SSL | Settings + file transfers unencrypted | Use network isolation + VPN | v2.0 | All |
| Multicast requires same subnet | Cross-subnet requires gossip or manual peers | Use ZeroTier (works well) | N/A (by design) | All |
| No HTTP authentication | Anyone on network can access HTTP server | Use Python API bridge (dglan-api) | v2.0 | All |
| GUI requires display | Remote headless setups can't use GUI | Use Core standalone; access via HTTP API | By design | Linux |
| Parallel qmake builds fail on Linux | Build races cause archive/moc errors | Use `make -j1` at top level | v1.4 | Linux only |
| Linux distro-specific binaries | Tarball only runs on similar distro/arch | Build natively on target platform | By design | Linux |

### Platform-Specific Known Issues

**Windows:**
- Auto-update downloads from GitHub Releases; requires internet connectivity
- Service runs as NETWORK SERVICE; check ACLs if shared folders fail

**Linux:**
- Distro packages (Qt5, protobuf ABI) must match build environment
- Multicast on VPN interfaces (ZeroTier) may require manual TTL tuning
- The shipped service is system-level and expects a dedicated `dglan` user to exist before enable/start
- SELinux may block multicast; check logs if peer discovery fails

---

## Release & Upgrade Procedures

### Checking for Updates

**Automatic (enabled by default):**
- DG-LAN checks GitHub Releases on startup
- If update available, GUI shows notification

**Manual check:**
- Windows GUI: Help → Check for Updates
- Linux: Manually check GitHub Releases or use auto-check if GUI running

### Upgrading

**Windows:**

1. Download latest `.exe` from [GitHub Releases](https://github.com/rwengine/DG-LAN/releases)
2. Run installer — auto-stops service, upgrades in-place, restarts service
3. Verify: `Get-Service "DG-LAN Core"` should show Running

**Linux:**

1. Download tarball matching your distro/arch from GitHub Releases
2. Stop service: `sudo systemctl stop dglan-core` (if systemd) or kill standalone process
3. Extract new tarball over existing installation:
   ```bash
   tar xzf DG-LAN-<version>-<tag>-linux-x86_64.tar.gz
   cd DG-LAN-<version>-<tag>-linux-x86_64/
   sudo ./install.sh /usr/local  # or your chosen prefix
   ```
4. Start service: `sudo systemctl start dglan-core`
5. Verify: `sudo systemctl status dglan-core`

### Post-Upgrade Validation

Both platforms retain settings and cache across upgrades. Verify:

**Windows:**
```powershell
# Service running and accepting connections
Get-Service "DG-LAN Core"
(netstat -ano | Select-String ":59480" | Measure-Object).Count -gt 0

# Check logs for errors
Get-Content "$env:LOCALAPPDATA\DGLan\Logs\*.log" -Tail 20
```

**Linux:**
```bash
# Service running and accepting connections
sudo systemctl status dglan-core
netstat -tlnp | grep 59480

# Check systemd logs for errors
sudo journalctl -u dglan-core -n 20
```

### Pre-Release Platform Testing

Before deploying to production:

**Windows:**
1. Run installer in test VM or staging machine
2. Verify Core service starts: `Get-Service "DG-LAN Core"`
3. Peer discovery works (multicast or manual peers)
4. HTTP server responds: `curl http://localhost:59480/`
5. Basic file share/download works in GUI

**Linux (per distro/arch):**
1. Extract tarball on target distro/arch
2. Run binary smoke test: `./DG-LAN.Core --version` (or `/usr/local/bin/DG-LAN.Core --version` after install)
3. Run systemd service integration test (if applicable)
4. Peer discovery works (multicast or manual peers)
5. HTTP server responds: `curl http://localhost:59480/`
6. Basic file share/download works in standalone Core (or GUI if display available)

See [BUILD.md — Validating Your Build](BUILD.md#validating-your-build) for full validation workflow.

---

## Maintenance Checklist

### Daily (for 24/7 deployments)

- [ ] Service running: Windows: `Get-Service "DG-LAN Core"`; Linux: `sudo systemctl status dglan-core`
- [ ] No errors in logs: Windows: `Get-Content "$env:LOCALAPPDATA\DGLan\Logs\*.log" -Tail 100`; Linux: `sudo journalctl -u dglan-core -n 100`
- [ ] Peer count > 0 (check GUI or HTTP API)
- [ ] Download queue progressing (check GUI or logs)

### Weekly

- [ ] Log file size not excessive (`%LOCALAPPDATA%\DGLan\Logs\`)
- [ ] Disk space available on target shared folders (>10% free)
- [ ] Memory usage stable (<300 MB)
- [ ] Check for available updates: Help → Check for Updates

### Monthly

- [ ] Backup settings file
- [ ] Archive old logs
- [ ] Performance review (slow peers? network congestion?)
- [ ] Shared folder audit (remove obsolete files, keep quota under control)

### Quarterly

- [ ] Review security configuration (firewall rules, password)
- [ ] Test disaster recovery (can you restore from backup?)
- [ ] Plan for version upgrades

---

## Related Documents

- **[BUILD.md](BUILD.md)** — Installation, build instructions (Windows primary, Linux experimental, multicast routing)
- **[ARCHITECTURE.md](ARCHITECTURE.md)** — Technical design, subsystem details, state machines
- **[README.md](README.md)** — User-facing overview, quick start
- **[PROJECT-CONTEXT.md](PROJECT-CONTEXT.md)** — Version system, CI/CD pipeline, modernization roadmap
- **[HTTP-SERVER.md](HTTP-SERVER.md)** — HTTP API and built-in server details
- **[TESTING.md](TESTING.md)** — Validation workflow, test suites, exit codes
