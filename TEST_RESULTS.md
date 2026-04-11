# DG-LAN — Test Results

**Last updated:** April 2026  
**Version:** 1.2.86 Alpha  
**Platform:** Windows 10/11 x64 (MSYS2 MinGW64 runtime)

---

## Build Verification

| Item | Status |
|------|--------|
| Core builds without error | PASS |
| GUI builds without error | PASS |
| Installer builds without error (Inno Setup 6) | PASS |
| App launches after install | PASS |
| Title bar shows "DG-LAN" | PASS |

---

## Core Functionality

| Feature | Status | Notes |
|---------|--------|-------|
| Peer discovery (multicast) | PASS | Peers found automatically on same subnet |
| Peer discovery (broadcast fallback) | PASS | Triggers when multicast has no response |
| Peer discovery (subnet scan) | PASS | Probes /24 when broadcast fails |
| Peer discovery (gossip/PEX) | PASS | Peers exchange neighbour lists |
| Core ↔ GUI connection (localhost) | PASS | TCP :59485 |
| File indexing and hashing | PASS | Scans shared directories on startup |
| Cache persistence | PASS | `file_cache.bin` saved/restored across restarts |
| Selective rehosting | PASS | Only downloads and pre-existing files are re-shared |
| Multi-source downloads | PASS | Pulls from all peers that have the file |

---

## GUI Functionality

| Feature | Status | Notes |
|---------|--------|-------|
| Unified file index (NetworkWidget) | PASS | All peers' files in one table |
| Peer list with priority | PASS | Right-click → High/Normal/Low |
| Connection speed display | PASS | LAN speed in peer tooltips |
| Search across all peers | PASS | Indexed search |
| Download queue with numbered positions | PASS | Sequential 1, 2, 3... for active downloads |
| Queue reorder (toolbar buttons) | PASS | ⏫ ▲ ▼ ⏬ buttons |
| Queue reorder (context menu) | PASS | Right-click → Move to Top/Up/Down/Bottom |
| Download/Delete/Redownload buttons | PASS | Toolbar with selection-aware enable/disable |
| Settings dialog (menu bar) | PASS | File → Settings opens dialog |
| Activity log (dock) | PASS | Real-time: scan, hash, download, peer, network events |
| Auto-update check | PASS | Checks GitHub Releases on launch |
| Update dialog with download progress | PASS | Downloads installer, runs silently |
| Forced update (protocol mismatch) | PASS | Modal dialog, must update or quit |
| Scrolling notification banner | PASS | Marquee when update available |
| `dglan://` URL scheme (IPC) | PASS | Links forwarded to running instance |
| System tray icon | PASS | Show/Exit context menu |
| About dialog | PASS | Version, tagline, contributors |
| Filter/search bar in file index | PASS | Real-time text filter |

---

## Known Issues

| Severity | Description |
|----------|-------------|
| Cosmetic | About dialog logo dark on dark background |

---

## Build Information

- **Repository:** https://github.com/kwran420/DG-LAN
- **Toolchain:** GCC (MinGW64), Qt 5.15.x, Protobuf 3.x
- **Build script:** `.\build-release.ps1`
