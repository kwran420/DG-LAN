# DG-LAN

**DG-LAN** decentralises a master file list across a network.  
One machine curates the files; every other machine helps distribute them — at full switch speed, with zero configuration.

Forked from [D-LAN](https://github.com/Ummon/D-LAN) and rebuilt with a modern UI, robust networking, and a master/client architecture.

> **Built for:** LAN parties, lab environments, office deployments — anywhere you need a designated set of files available to everyone on a network.  
> Optionally works across subnets via [ZeroTier](https://www.zerotier.com/).

---

## How it works

1. **Master curates** — one machine runs in master mode, shares designated folders, and indexes every file in them.
2. **Clients join** — other machines connect, browse the master file list, and download what they need.
3. **Clients rehost** — every file a client downloads is automatically shared back to the network, distributing the load across all peers.
4. **Discovery is automatic** — peers find each other via multicast, broadcast, subnet scan, and gossip. No manual IP entry.

The result: a single authoritative file list, distributed across every machine that participates. The more people download, the faster it gets for everyone.

---

## Features

| Category | Feature |
|----------|---------|
| **Master List** | Master indexes all shared folders; clients see and download from this curated file list |
| **Auto-Rehosting** | Downloaded files are automatically re-shared — the network becomes a distributed mirror |
| **Discovery** | Multicast, directed broadcast, subnet scan (`/24`), gossip / peer exchange (PEX) — fully automatic fallback chain |
| **File Browsing** | Unified file index in one table — browse the entire master list at a glance |
| **Downloads** | Multi-source transfers pull from all peers that have the file, maximizing speed |
| **Queue Management** | Download queue with reorder buttons (⏫ ▲ ▼ ⏬) and right-click menu |
| **Search** | Fast indexed search across the file list |
| **Master Password** | Password-protect the network so only authorised clients can join |
| **Auto-Update** | Checks GitHub Releases on launch; downloads and installs updates in-app |
| **Forced Update** | Protocol version mismatch triggers a mandatory update dialog |
| **Logging** | Real-time activity log: scanning, hashing, transfers, peer join/leave |
| **`dglan://` URLs** | Custom URL scheme for one-click downloads from web pages |
| **ZeroTier** | Bind to a ZeroTier interface for cross-subnet use |
| **Remote Control** | GUI can connect to a Core running on another machine |
| **Headless Core** | Core runs as a Windows service — no GUI required on always-on machines |

---

## Quick Start

### Master (the machine with the files)
1. **Install** DG-LAN (download from [GitHub Releases](https://github.com/kwran420/DG-LAN/releases)).
2. **Add shared folders** in Settings → Preferences — these are the files everyone will download.
3. **Set a master password** (optional) to restrict who can join.
4. Leave it running — the master indexes your files and serves the list.

### Clients (everyone else)
1. **Install** DG-LAN and launch it.
2. **Enable client mode** in Settings → Preferences → Client mode.
3. **Browse & download** from the master file list.
4. Files you download are automatically rehosted to other clients.

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
