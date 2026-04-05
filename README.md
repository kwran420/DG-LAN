# DG-LAN

**DG-LAN** is a fork of [D-LAN](https://github.com/Ummon/D-LAN) — an open-source, decentralised LAN file-sharing application.  
The fork adds reliable peer discovery, core-seeder support, and a `dglan://` URL scheme for one-click downloads from web pages.

> **Primary use case:** local area networks (LAN parties, home networks, office networks).  
> ZeroTier is supported as an optional overlay for testing or cross-subnet use.

---

## Features

### Inherited from D-LAN
- Share files and folders on a LAN with no central server and no configuration.
- Distributed (multi-source) transfers for higher speed and resilience.
- Fast indexed search and full peer browse.
- Download queue management (add, delete, pause, reorder).
- Global persistent chat with channels.
- Headless Core daemon + optional remote GUI control.
- Open source under GPLv3.

### Added in DG-LAN
- **Robust peer discovery fallback chain**
  1. IPv4 multicast (original D-LAN mechanism)
  2. Directed broadcast (subnet-wide `x.x.x.255`)
  3. Subnet scan (probes every host in the `/24`)
  4. Gossip / Peer Exchange (PEX) — peers share their neighbour lists
- **Core Seeder mode** — designate always-on machines as reliable seeders; other peers probe them first at startup before running a full scan.
- **`dglan://` URL scheme** — clicking a `dglan://` link in a browser instantly queues a download in the already-running DG-LAN instance (no copy-paste required).
- **ZeroTier support** — optionally bind to a named network interface (e.g. a ZeroTier adapter) for overlay-network use.
- **Configurable network settings** — interface name, multicast TTL, force-IPv4, and gossip parameters exposed in the GUI settings dialog.

---

## Quick Start

1. Build or install DG-LAN on every machine (see [BUILD.md](BUILD.md)).
2. **Same subnet (typical LAN):** launch and go — peers find each other automatically via multicast/broadcast.
3. **Cross-subnet / ZeroTier:** set **Settings → Network → Interface** to the overlay adapter on each machine.
4. **Core Seeder:** tick **"This machine is a Core Seeder"** on the always-on machine; add its IP under **Settings → Network → Core Seeders** on the other machines.

---

## `dglan://` URL Scheme

DG-LAN registers a custom URL protocol so that any web page can link directly into the app:

```
dglan://download?peer=PEER_HEX&hash=ENTRY_HEX&size=BYTES&name=FILENAME&path=/
```

| Parameter | Description |
|-----------|-------------|
| `peer`    | 56-char hex peer ID (from Browse / Search results) |
| `hash`    | 56-char hex shared-entry ID |
| `size`    | File size in bytes |
| `name`    | Percent-encoded filename |
| `path`    | Percent-encoded path inside the shared entry (`/` for root) |

When a `dglan://` link is clicked:
- If DG-LAN is already running, the link is forwarded via an IPC channel and the download starts immediately.
- If DG-LAN is not running, it starts, connects to the Core, then triggers the download.

**Register the scheme** on your OS — see the [URL Scheme Registration](BUILD.md#url-scheme-registration-dglan) section in BUILD.md.

### Minimal HTML example
```html
<a href="dglan://download?peer=0011aabb...&hash=ccdd1122...&size=104857600&name=movie.mkv&path=/">
    Download with DG-LAN
</a>
```

---

## Peer Discovery Details

| Method | When it runs |
|--------|-------------|
| Multicast (224.0.0.1:59486) | Always — same as original D-LAN |
| Directed broadcast | Fallback when no multicast response within ~2 s |
| Subnet scan (`/24`) | Fallback when no broadcast response within ~4 s |
| Gossip / PEX | Continuously after first peer contact; peers exchange neighbour lists |

Gossip entries expire after **30 minutes** by default (configurable). The peer list is capped at **50 gossip candidates** to avoid memory growth.

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  GUI (Qt Widgets)                                        │
│  ┌──────────────────────────────────────────────────┐   │
│  │  D_LAN_GUI  ←  dglan:// URLs (QLocalServer IPC)  │   │
│  │  MainWindow, Settings, Tray icon                  │   │
│  └──────────────────────────────┬───────────────────┘   │
│                                  │ TCP 59485             │
└──────────────────────────────────┼──────────────────────┘
                                   │
┌──────────────────────────────────▼──────────────────────┐
│  Core (headless daemon)                                  │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │ PeerManager │  │ UDPListener  │  │ FileManager    │  │
│  │ + Gossip    │  │ + Discovery  │  │ + Indexer      │  │
│  │   PEX       │  │   fallback   │  │ + Downloader   │  │
│  └─────────────┘  └──────────────┘  └────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## Ports Used

| Port  | Protocol  | Purpose                              |
|-------|-----------|--------------------------------------|
| 59486 | UDP       | Multicast / broadcast peer discovery |
| 59487 | UDP + TCP | Unicast peer communication           |
| 59485 | TCP       | Remote GUI control                   |

Ports 59486–59497 are used in practice (increments if busy). Allow these through your OS firewall — see [BUILD.md](BUILD.md) for exact commands.

---

## Building

See **[BUILD.md](BUILD.md)** for full platform-specific instructions.

**Short version (Linux):**
```bash
sudo apt-get install -y qt5-default qtbase5-dev libprotobuf-dev protobuf-compiler build-essential
cd application
qmake D-LAN.pro && make -j$(nproc)
```

**Short version (Windows — Developer Command Prompt for VS 2019):**
```bat
cd application
qmake D-LAN.pro && nmake
```

---

## Contributing

1. Fork and clone the repo.
2. Build with the instructions above.
3. Open a PR — the GitHub Actions workflow will run a Linux build check automatically.

---

## License

GPLv3 — see the original [D-LAN licence](https://github.com/Ummon/D-LAN) for details.
