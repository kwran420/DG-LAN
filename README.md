# DG-LAN

**DG-LAN** is a decentralized LAN file-sharing application for Windows.  
Drop it on every machine, launch it, and instantly see and download everyone's shared files — no server, no setup, no accounts.

Forked from [D-LAN](https://github.com/Ummon/D-LAN) and rebuilt with a modern, streamlined UI and robust networking.

> **Built for:** LAN parties, home networks, office file sharing.  
> Optionally works across subnets via [ZeroTier](https://www.zerotier.com/).

---

## What it does

1. **Share** — point DG-LAN at folders on your machine.
2. **Discover** — every peer on the LAN appears automatically (multicast → broadcast → subnet scan → gossip).
3. **Browse** — a single unified file index shows every file from every peer, with search and filtering.
4. **Download** — multi-source transfers pull from all peers that have the file, maximizing speed.
5. **Rehost** — downloaded files are automatically shared back to the network (selective rehosting — only files you downloaded or already had in shared folders).

No central server. No configuration. Just plug in and go.

---

## Features

| Category | Feature |
|----------|---------|
| **Discovery** | Multicast, directed broadcast, subnet scan (`/24`), gossip / peer exchange (PEX) — fully automatic fallback chain |
| **File Browsing** | Unified file index across all peers in one table — no per-peer tabs |
| **Downloads** | Multi-source distributed transfers, download queue with drag reorder (⏫ ▲ ▼ ⏬ toolbar buttons + right-click menu) |
| **Rehosting** | Selective — only downloaded files and existing shared files are re-shared; externally added files are not auto-indexed |
| **Peer Priority** | Right-click any peer to set High / Normal / Low priority |
| **Connection Speed** | Peers advertise their LAN speed (displayed in peer tooltips) |
| **Search** | Fast indexed search across all peers |
| **Auto-Update** | Checks GitHub Releases on launch; downloads and installs updates in-app |
| **Forced Update** | Network protocol version mismatch triggers a mandatory update dialog |
| **Logging** | Real-time activity log: scanning, hashing, download start/complete/cancel, peer join/leave, network refresh |
| **Scrolling Notification** | Marquee bar when an update is available |
| **`dglan://` URLs** | Custom URL scheme for one-click downloads from web pages |
| **ZeroTier** | Bind to a ZeroTier interface for cross-subnet use |
| **Remote Control** | GUI can connect to a Core running on another machine |
| **Headless Core** | Core runs as a Windows service — no GUI required on always-on machines |

---

## Quick Start

1. **Install** DG-LAN on every machine (download the installer from [GitHub Releases](https://github.com/kwran420/DG-LAN/releases)).
2. **Launch** — peers find each other automatically on the same subnet.
3. **Share** folders via Settings → Preferences.
4. **Browse & download** from the unified file index.

For cross-subnet / ZeroTier setups, see [BUILD.md](BUILD.md#zerotier-setup).

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  GUI  (Qt5 Widgets)                                      │
│  ┌────────────────────────────────────────────────────┐  │
│  │ MainWindow                                         │  │
│  │  ├─ NetworkWidget  (unified file index + peers)    │  │
│  │  ├─ Log dock       (real-time activity log)        │  │
│  │  ├─ Settings       (menu bar → dialog)             │  │
│  │  ├─ UpdateChecker  (GitHub Releases auto-update)   │  │
│  │  └─ ScrollingNotification  (update banner)         │  │
│  └──────────────────────┬─────────────────────────────┘  │
│                          │ TCP localhost:59485            │
└──────────────────────────┼───────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────┐
│  Core  (headless daemon / Windows service)                │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │ PeerManager  │  │ UDPListener  │  │ FileManager    │  │
│  │  + Gossip    │  │  + Multicast │  │  + Indexer     │  │
│  │    PEX       │  │  + Broadcast │  │  + Cache       │  │
│  │              │  │  + Scan      │  │  + Downloader  │  │
│  └──────────────┘  └──────────────┘  └────────────────┘  │
└──────────────────────────────────────────────────────────┘
```

---

## Peer Discovery

Peers are found automatically via a fallback chain:

| Method | When |
|--------|------|
| Multicast (`224.0.0.1:59486`) | Always — primary mechanism |
| Directed broadcast (`x.x.x.255`) | Fallback when no multicast response ~2 s |
| Subnet scan (`/24`) | Fallback when no broadcast response ~4 s |
| Gossip / PEX | Continuously after first peer contact; peers share neighbour lists |

Gossip entries expire after 30 minutes (configurable). The peer list is capped at 50 gossip candidates.

---

## Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 59485 | TCP | GUI ↔ Core remote control |
| 59486 | UDP | Multicast / broadcast peer discovery |
| 59487 | UDP + TCP | Unicast peer communication |

Ports increment if busy (range 59486–59497). See [BUILD.md](BUILD.md#firewall) for firewall rules.

---

## `dglan://` URL Scheme

Web pages can link directly into DG-LAN:

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

If DG-LAN is running, the download starts immediately via IPC. If not, DG-LAN launches and queues the download after connecting to Core.

Registration instructions: [BUILD.md — URL Scheme Registration](BUILD.md#url-scheme-registration-dglan).

---

## HTTP API Bridge

The `dglan-api/` directory contains a standalone Python HTTP server that connects to a running Core and serves shared file listings as JSON — so a website can generate `dglan://` download links dynamically. See [dglan-api/README.md](dglan-api/README.md).

---

## Building from Source

See **[BUILD.md](BUILD.md)** for full instructions.

**Quick version (Windows — the primary platform):**
```powershell
# Requires: MSYS2 with MinGW64 packages (Qt5, protobuf, GCC) + Inno Setup 6
.\build-release.ps1              # build + publish to GitHub Releases
.\build-release.ps1 -SkipPublish # build only, no push
```

---

## Contributing

1. Fork and clone the repo.
2. Build with `.\build-release.ps1 -SkipPublish`.
3. Open a PR describing what changed, why, and how it was tested.

---

## License

GPLv3 — see [COPYING](COPYING) for the full text.  
Original D-LAN by Greg Burri: [github.com/Ummon/D-LAN](https://github.com/Ummon/D-LAN).
