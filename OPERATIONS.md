# DG-LAN — Operations & Deployment Guide

Runbook for system administrators, operations teams, and users deploying DG-LAN in production or lab environments.

**Target audience:** DevOps, sysadmins, lab operators, advanced end-users running 24/7 instances.

---

## Table of Contents

1. [Running as Windows Service](#running-as-windows-service)
2. [Configuration & Settings](#configuration--settings)
3. [Logging & Monitoring](#logging--monitoring)
4. [Troubleshooting](#troubleshooting)
5. [Performance Tuning](#performance-tuning)
6. [Backup & Recovery](#backup--recovery)
7. [Security Considerations](#security-considerations)
8. [Known Limitations](#known-limitations)

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

## Configuration & Settings

### Default Installation Paths

#### Windows

| Component | Path |
|-----------|------|
| Program files | `C:\Program Files\DG-LAN\` |
| Roaming data (settings) | `%APPDATA%\DGLan\` |
| Local data (cache, logs, queue) | `%LOCALAPPDATA%\DGLan\` |

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
- **Core (standalone):** Logs printed to console

#### On-Disk Log Files

- **Path:** `%LOCALAPPDATA%\DGLan\Logs\` (Windows)
- **Format:** Plain text, timestamped
- **Rotation:** Not automatic — files grow indefinitely

**Manual log cleanup (on server with 24/7 uptime):**

```powershell
# Archive old logs (keep last 30 days)
$cutoffDate = (Get-Date).AddDays(-30)
Get-ChildItem "C:\Users\<user>\AppData\Local\DGLan\Logs" -Filter "*.log" |
    Where-Object { $_.LastWriteTime -lt $cutoffDate } |
    Move-Item -Destination "C:\Logs\DGLan\Archive\"

# Or delete old logs:
Get-ChildItem "C:\Users\<user>\AppData\Local\DGLan\Logs" -Filter "*.log" |
    Where-Object { $_.LastWriteTime -lt $cutoffDate } |
    Remove-Item
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

### Peer Discovery Not Working

**Symptom:** Core is running but no peers appear in the peer list.

**Root causes & solutions:**

| Cause | Solution |
|-------|----------|
| Firewall blocking UDP :59486 | Add firewall rule (see [Firewall Configuration](#firewall-configuration)) |
| Wrong network interface | Settings → Network → Interface (manually select correct NIC) |
| Multicast disabled | Try manual broadcast/subnet scan (auto-fallback should work) |
| Different subnets | Use ZeroTier (see [BUILD.md → ZeroTier Setup](BUILD.md#zerotier-setup)) |
| Core service not running | `net start "DG-LAN Core"` |
| Multicast TTL too low | Set to `1` for LAN; higher for ZeroTier |

**Diagnostic steps:**

```powershell
# 1. Verify Core is running
Get-Service "DG-LAN Core"

# 2. Verify port is listening
netstat -ano | Select-String "59486"

# 3. Check settings for correct interface
# (Open GUI Settings → Network)

# 4. Check logs for discovery errors
Get-Content "$env:LOCALAPPDATA\DGLan\Logs\*.log" -Tail 50
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

**Via command line (not implemented yet):**
- Edit `settings.xml` directly: `<interface>ZeroTier One [...]</interface>`

**Best practice:**
- Prefer wired (Ethernet) for stable peer discovery
- If using ZeroTier, select the ZeroTier virtual interface explicitly

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

| Item | Location | Importance | Frequency |
|------|----------|-----------|-----------|
| Settings | `%APPDATA%\DGLan\settings.xml` | HIGH | Daily |
| File cache | `%LOCALAPPDATA%\DGLan\file_cache.bin` | MEDIUM | Daily (auto-rebuilt on loss) |
| Shared files | User-defined folders | HIGH | Per deployment policy |

### Backup Strategy

**Minimal (settings only):**
```powershell
$backup = "C:\Backups\DGLan_$(Get-Date -Format 'yyyyMMdd').zip"
Compress-Archive -Path "$env:APPDATA\DGLan\settings.xml" -DestinationPath $backup
```

**Full (settings + cache):**
```powershell
$backup = "C:\Backups\DGLan_$(Get-Date -Format 'yyyyMMdd').zip"
Compress-Archive -Path "$env:APPDATA\DGLan", "$env:LOCALAPPDATA\DGLan\file_cache.bin" -DestinationPath $backup
```

### Recovery

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

### Cache Rebuild

If `file_cache.bin` is corrupted or lost:

1. Delete or rename the file
2. Restart Core — automatic full scan and rebuild (may take several minutes)

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

| Limitation | Impact | Workaround | Target Fix |
|-----------|--------|-----------|-----------|
| No automatic log rotation | Logs grow unbounded; may fill disk | Manual cleanup (see [Logging](#logging--monitoring)) | v1.3 |
| No IPv6 support | IPv6-only networks not supported | Use dual-stack or IPv4-only network | v2.0 |
| Selective rehosting in client mode | Fewer file sources, slower downloads | Master mode has better performance | By design |
| No bandwidth limiting | Fast peer can saturate network | Use OS QoS or limit TCP window | v1.4 |
| Single password (no user roles) | Coarse access control | Rely on firewall + network isolation | v2.0 |
| No TLS/SSL | Settings + file transfers unencrypted | Use network isolation + VPN | v2.0 |
| Multicast requires same subnet | Cross-subnet requires gossip or manual peers | Use ZeroTier (works well) | N/A (by design) |
| No HTTP authentication | Anyone on network can access HTTP server | Use Python API bridge (dglan-api) | v2.0 |

---

## Maintenance Checklist

### Daily (for 24/7 deployments)

- [ ] Service running: `Get-Service "DG-LAN Core"`
- [ ] No errors in logs: `Get-Content "$env:LOCALAPPDATA\DGLan\Logs\*.log" -Tail 100`
- [ ] Peer count > 0 (check GUI)
- [ ] Download queue progressing (check GUI)

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

- **[BUILD.md](BUILD.md)** — Installation and build instructions
- **[ARCHITECTURE.md](ARCHITECTURE.md)** — Technical design, subsystem details
- **[README.md](README.md)** — User-facing overview
- **[PROJECT-CONTEXT.md](PROJECT-CONTEXT.md)** — Version system, CI/CD pipeline
- **[HTTP-SERVER.md](HTTP-SERVER.md)** — HTTP API and built-in server details
