# DG-LAN — Architecture & Design

DG-LAN decentralises a master file list across a network via a modular Core daemon and Qt5 GUI frontend. This document explains subsystem boundaries, key flows, concurrency patterns, and high-risk areas.

**Target audience:** Core contributors, system designers, future maintainers.

---

## System Overview

```
┌─────────────────────────────────────────────────────────┐
│  DG-LAN GUI  (Qt5 Widgets)                              │
│  ┌────────────────────────────────────────────────────┐  │
│  │ MainWindow                                         │  │
│  │  ├─ NetworkWidget  (unified file index + peers)    │  │
│  │  ├─ Log dock       (real-time activity log)        │  │
│  │  ├─ Settings       (shared folder + peer mgmt)     │  │
│  │  ├─ UpdateChecker  (GitHub Releases auto-update)   │  │
│  │  └─ ScrollingNotification  (marquee banner)        │  │
│  └──────────────────────┬─────────────────────────────┘  │
│                          │ TCP localhost:59485            │
│                          │ Protobuf + MessageHeader       │
└──────────────────────────┼───────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────┐
│  DG-LAN Core  (headless daemon / Windows service)        │
│                                                           │
│  ┌───────────────────┐  ┌────────────────────┐           │
│  │ PeerManager       │  │ NetworkListener    │           │
│  │ + ConnectionPool  │  │  + UDPListener     │           │
│  │ + Peer contacts   │  │  + Multicast       │           │
│  │ + Peer-to-peer    │  │  + Broadcast       │           │
│  │   messaging       │  │  + Subnet scan     │           │
│  │                   │  │  + Gossip (PEX)    │           │
│  └────────┬──────────┘  └────────┬───────────┘           │
│           │                      │                        │
│           └──────┬───────────────┘                        │
│                  │ (peer discovery,                       │
│                  │  gossip exchange)                      │
│                  │                                        │
│  ┌───────────────▼──────────────────────────────────┐   │
│  │ FileManager                                       │   │
│  │ ├─ FileUpdater (background scan thread)          │   │
│  │ ├─ Cache (directory tree + entry metadata)       │   │
│  │ ├─ WordIndex (search)                            │   │
│  │ ├─ HashCache + ChunkIndex                        │   │
│  │ └─ file_cache.bin (protobuf persistence)         │   │
│  └───────────┬──────────────────────────────────────┘   │
│              │                                           │
│  ┌───────────▼──────────────────────────────────────┐   │
│  │ DownloadManager + UploadManager                  │   │
│  │ ├─ Download queue (multi-source transfers)       │   │
│  │ ├─ Priority scheduling (High/Normal/Low peers)   │   │
│  │ └─ Rate limiting                                 │   │
│  └───────────────────────────────────────────────────┘   │
│              │                                           │
│  ┌───────────▼──────────────────────────────────────┐   │
│  │ HttpServer (port 59480)                          │   │
│  │ ├─ File streaming with Range requests            │   │
│  │ ├─ Peer redirect (get file from other peer)      │   │
│  │ └─ CORS + ETag caching                           │   │
│  └───────────────────────────────────────────────────┘   │
│              │                                           │
│  ┌───────────▼──────────────────────────────────────┐   │
│  │ RemoteControlManager (GUI control)               │   │
│  │ ├─ TCP listener :59485                           │   │
│  │ ├─ Protobuf message dispatch                     │   │
│  │ └─ Browse + settings updates                     │   │
│  └───────────────────────────────────────────────────┘   │
│              │                                           │
│  ┌───────────▼──────────────────────────────────────┐   │
│  │ ChatSystem (DEFUNCT — consider pruning)         │   │
│  │ ├─ Peer chat messaging                           │   │
│  │ └─ (not actively used in 1.2.x)                  │   │
│  └───────────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────┘
```

---

## Module Ownership & Boundaries

### 1. FileManager (Core/FileManager/)

**Responsibility:** Index shared directories, cache metadata, search, hash tracking.

**Key classes:**
- `FileManager` — main interface; `IFileManager` defines public contract
- `FileUpdater` — runs in separate thread, scans directories continuously
- `Cache` — in-memory tree of `Directory*` and `File*` (raw Qt parent-child pointers)
- `file_cache.bin` — protobuf binary written every 60s via `timerPersistCache`
- `WordIndex` — full-text search on file names
- `Chunks` — fine-grained hash chunks for partial file sharing

**Concurrency model:**
- `FileUpdater` runs in its own thread; all accesses to `Cache` use `QMutex`
- Master mode: full recursive scan every ~20s
- Client mode: selective rehosting — only rescans on file watcher events or download completion

**High-risk areas:**
- **Cache invalidation:** If a file is deleted externally while a download is in progress, `Cache` may hold a stale entry. Mitigation: `FileUpdater` re-scans on change; downloads check file existence before opening.
- **Memory overhead:** Large shared directories (>500K files) load entire tree into memory. No pagination. Consider streaming walk for future versions.
- **Hash collision:** ChunkIndex splits files into 16 KB chunks; very small files or highly repetitive data could hash collide. Low risk in practice (MD5, collision unlikely to matter).

**Tested:** TestsFileManager suite (partial).

---

### 2. PeerManager (Core/PeerManager/)

**Responsibility:** Maintain peer list, establish connections, exchange metadata, detect peer failure.

**Key classes:**
- `PeerManager` — main manager, maintains `ConnectionPool` + gossip candidates
- `Peer` — in-memory peer state + message socket
- `PeerSelf` — special peer representing the local machine
- `ConnectionPool` — manages `PeerMessageSocket` lifecycle, auto-reconnect
- `UDPListener` (in NetworkListener) — discovery probing and gossip reception

**Concurrency model:**
- All peer state protected by `QMutex` in `PeerManager`
- `PeerMessageSocket` uses signal/slots to dispatch incoming messages to `Peer`
- Connections are long-lived TCP; reconnection is automatic

**Peer lifecycle signals:**
- `peerBecomesAvailable(IPeer*)` — emitted when a peer becomes alive or unblocked
- `peerBecomesUnavailable(IPeer*)` — emitted when a peer dies (timeout or removal); consumers must proactively clean up cached references
- `Peer::becameDead()` — internal signal forwarded by PeerManager

**Peer discovery flow:**
1. Multicast probe (`224.0.0.1:59486`) — primary, fires every ~5s
2. ~2s timeout → Directed broadcast (`x.x.x.255:59486`)
3. ~2s timeout → Subnet scan (`/24` or `/16` if user configured)
4. Gossip / PEX — after first peer contact, peers share neighbour lists
5. Gossip entries expire after 30 minutes (configurable); list capped at 50 candidates

**High-risk areas:**
- **NAT traversal:** No UPnP/hole-punch. Multicast + subnet scan assume LAN. Cross-subnet use requires manual relay (ZeroTier or static peer list).
- **Peer flooding:** If a peer sends many gossip entries, the list could grow unbounded. Mitigation: 50-entry cap; oldest entries expire.
- **Connection loss detection:** TCP keepalive relies on OS TCP_KEEPALIVE. Slow network hangs may not be detected for minutes. Consider app-level heartbeat in v1.3+.

**Tested:** TestsPeerManager suite.

---

### 3. DownloadManager (Core/DownloadManager/)

**Responsibility:** Queue downloads, manage multi-source transfers, schedule peer priority.

**Key classes:**
- `DownloadManager` — main manager; maintains active + queued downloads
- `FileDownload` — per-file state machine (queued → downloading → completed/failed)
- `DownloadQueue` — priority queue; reorderable by GUI
- Priority levels: High / Normal / Low (affects scheduler weight)

**Concurrency model:**
- Download events (start, progress, complete, error) are thread-safe signals
- File I/O happens in a thread pool; disk write order is not guaranteed
- Rate limiting can pause individual transfers

**Download flow:**
1. User queues file → `FileDownload` created, inserted at tail of queue
2. GUI can reorder queue (⏫ ▲ ▼ ⏬ buttons)
3. Scheduler picks next download based on peer priority + bandwidth
4. Multi-source: if 3 peers have the file, fetch in parallel from all (segment-wise or full-copy)
5. On completion, file is moved to shared folder and indexed by FileManager

**High-risk areas:**
- **Raw IPeer\* pointers:** DownloadManager and ChunkDownloader store raw `PM::IPeer*` pointers. Peers are not deleted during normal operation (only at shutdown), but the `peerBecomesUnavailable` signal now enables proactive cleanup. Full migration to `QSharedPointer<IPeer>` is planned (D7).
- **Partial download resume:** If download interrupted mid-file, resume logic is not fully hardened. Checksums may not align.
- **Disk space exhaustion:** No pre-flight disk space check. If target drive is full, download fails silently and queue stalls.
- **Queue persistence:** Queue is not persisted; restart loses pending downloads. Mitigation: QSettings stores queue state but not file paths.
- **Tests disabled:** DownloadManager tests are commented out; high regression risk. Re-enable in v1.3.

**Tested:** TestsDownloadManager suite (DISABLED in current build).

---

### 4. UploadManager (Core/UploadManager/)

**Responsibility:** Serve downloaded files back to peers, limit bandwidth.

**Key classes:**
- `UploadManager` — main manager
- Upload state machine (queued → sending → complete/failed)

**High-risk areas:**
- **Selective rehosting:** Client mode only re-shares files that were explicitly downloaded or already in shared folders. Logic is split between FileManager (master vs. client mode) and upload path. Easy to misunderstand or break. Consider dedicated "RehostedFileSet" in v1.3+.

**Tested:** Partially (integration tests only).

---

### 5. NetworkListener (Core/NetworkListener/)

**Responsibility:** Receive peer discovery probes (multicast, broadcast, subnet scan), send gossip.

**Key classes:**
- `UDPListener` — listens on UDP :59486, handles multicast + broadcast + scan
- Multicast join/leave per network interface
- Fallback chain: multicast → broadcast → scan → gossip

**High-risk areas:**
- **IPv6:** No IPv6 support. Multicast address is IPv4-only (`224.0.0.1`). IPv6 deployments will silently fall back to scan.
- **Network interface binding:** If machine has multiple NICs, multicast might bind to wrong interface. Manual selection via GUI available.
- **Scan overhead:** Subnet scan (`/24` or `/16`) sends TCP probes to every IP. On large subnets this can be slow (~10s for /16).

**Tested:** Integration tests; not unit-tested in isolation.

---

### 6. HttpServer (Core/HttpServer/)

**Responsibility:** Serve shared files via HTTP (port 59480), redirect requests for files not locally cached.

**Key classes:**
- `HttpServer` — main server, listens on :59480
- `HttpConnection` — per-client connection handler
- Full HTTP/1.1 support: GET, HEAD, Range requests (for video seeking)

**Features:**
- `Range: bytes=N-M` support (streaming video/audio with seeking)
- `ETag` caching (if-none-match 304)
- CORS headers
- Peer redirect: if this machine doesn't have the file, responds 3xx redirect to another peer's HTTP server

**High-risk areas:**
- **Path traversal:** All paths go through `FileManager`, which blocks `..` traversal. Safe.
- **Slow client:** If a client opens a Range request but never reads, connection stays open. No timeout. Could exhaust FD limit.
- **Bandwidth overload:** No rate limiting per client. A single remote connection could starve downloads.

**Tested:** Manual; no automated tests.

---

### 7. RemoteControlManager (Core/RemoteControlManager/)

**Responsibility:** Accept GUI connections (TCP :59485), deserialize protobuf messages, dispatch to other managers.

**Key classes:**
- `RemoteControlManager` — main manager
- `RemoteConnection` — per-GUI connection handler, protobuf marshalling

**Message flow:**
1. GUI sends `Common::MessageHeader` (type code + size) + protobuf payload
2. `RemoteConnection` deserializes based on type code
3. Dispatches to FileManager, PeerManager, DownloadManager, etc.
4. Result serialized and sent back

**High-risk areas:**
- **Protocol versioning:** No versioning negotiation. If GUI protocol changes, old clients will fail silently. Mitigation: force update on version mismatch.
- **Message size limits:** No length checks; malformed protobuf could cause OOM. Consider max message size in v1.3+.
- **Blocking calls:** GUI browse requests call `FileManager::getEntries()` synchronously. Large directories could block Core for seconds.

**Tested:** Integration tests.

---

### 8. ChatSystem (Core/ChatSystem/) — DEFUNCT

**Status:** Historical; used in D-LAN but not actively maintained in DG-LAN 1.2+.

**Recommendation:** Mark as deprecated in v1.2.x, plan removal in v2.0+. No users depend on it; pruning will reduce maintenance burden.

---

## Concurrency & Thread Safety

### Thread Model

**Core runs ~4 active threads:**
1. **Main Qt thread** — event loop, signal/slot dispatch, Core initialization
2. **FileUpdater thread** — continuous directory scanning (low CPU when idle)
3. **DownloadManager thread pool** — file I/O and transfers
4. **HttpServer thread** — Accepts TCP connections; each client connection spawns a handler

**GUI runs ~2 active threads:**
1. **Main Qt thread** — GUI rendering, user input
2. **UpdateChecker thread** — HTTP fetch to GitHub Releases

### Mutex Rules

- **FileManager::Cache** — always protected by `fileManagerMutex`
- **PeerManager::Peer list** — always protected by `peerManagerMutex`
- **DownloadManager::Queue** — always protected by `downloadManagerMutex`

### Signal/Slot Safety

- All Qt signals are implicitly thread-safe (Qt queues events)
- Cross-thread connections use `Qt::QueuedConnection` by default
- `QObject` deletion is safe only from the thread that owns it; use `deleteLater()` for cross-thread cleanup

### Known Issues

- **FileUpdater blocking:** If a folder contains millions of files, scan thread could lag. No progress indication to GUI.
- **Slow peer response:** If a remote peer responds slowly to a browse request, GUI freezes waiting for result. Async browsing considered for v1.3+.

---

## Key Flows

### Peer Discovery (Detailed State Machine)

```
┌──────────────────────────────────────────────────────┐
│ UDPListener::start()                                 │
│ ├─ Join multicast 224.0.0.1:59486                    │
│ ├─ Bind to :59486                                    │
│ └─ Start listening                                   │
└──────────────┬───────────────────────────────────────┘
               │
        [timer every 5s]
               │
┌──────────────▼───────────────────────────────────────┐
│ sendMulticastProbe()                                 │
│ └─ "Who is on the network?" → 224.0.0.1:59486       │
└──────────────┬───────────────────────────────────────┘
               │
        [responses arrive]
               │
┌──────────────▼───────────────────────────────────────┐
│ onPeerProbeResponse()                                │
│ ├─ Extract peer IP, port, peer ID                    │
│ ├─ Create Peer object if new                         │
│ └─ Start TCP connection in background                │
└──────────────┬───────────────────────────────────────┘
               │
        [if no responses in 2s]
               │
┌──────────────▼───────────────────────────────────────┐
│ sendBroadcastProbe()                                 │
│ └─ "Who is on my subnet?" → x.x.x.255:59486         │
└──────────────┬───────────────────────────────────────┘
               │
        [if no responses in 2s]
               │
┌──────────────▼───────────────────────────────────────┐
│ scanSubnet()                                         │
│ └─ For each IP in /24: TCP connect to :59487         │
└──────────────┬───────────────────────────────────────┘
               │
        [peers connect, send HELLO]
               │
┌──────────────▼───────────────────────────────────────┐
│ onPeerHello()                                        │
│ ├─ Extract peer ID + candidate peer list (gossip)   │
│ ├─ Add peers to gossip candidate list                │
│ └─ Send HELLO_ANSWER                                 │
└──────────────┬───────────────────────────────────────┘
               │
        [continuous PEX exchange]
               │
┌──────────────▼───────────────────────────────────────┐
│ Gossip / PEX:                                        │
│ ├─ Store up to 50 candidates from peers              │
│ ├─ Rotate through candidates                         │
│ ├─ Expire entries after 30 min                       │
│ └─ Attempt connection                                │
└──────────────────────────────────────────────────────┘
```

### File Download (Detailed)

```
┌──────────────────────────────────────────────────────┐
│ User clicks "Download" in GUI                        │
└──────────────┬───────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────────────────┐
│ FileDownload created                                 │
│ ├─ State: Queued                                     │
│ ├─ Add to DownloadManager::downloadQueue             │
│ └─ Position: tail (user can reorder)                 │
└──────────────┬───────────────────────────────────────┘
               │
        [when scheduler picks it]
               │
┌──────────────▼───────────────────────────────────────┐
│ DownloadManager::startFileDownload()                 │
│ ├─ State: Downloading                                │
│ ├─ Find all peers with this file                     │
│ └─ Initiate multi-source transfer                    │
└──────────────┬───────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────────────────┐
│ Segment-wise fetch from multiple peers:              │
│ ├─ Peer A: bytes 0-500K                              │
│ ├─ Peer B: bytes 500K-1M                             │
│ └─ Peer C: bytes 1M-...                              │
│                                                       │
│ Write to temp file: target/.dg-downloading           │
│ Report progress every 100ms                          │
└──────────────┬───────────────────────────────────────┘
               │
        [all segments received]
               │
┌──────────────▼───────────────────────────────────────┐
│ DownloadManager::onDownloadComplete()                │
│ ├─ Verify hash matches shared-entry hash             │
│ ├─ Rename .dg-downloading → final filename           │
│ ├─ Notify FileManager                                │
│ └─ FileManager indexes file, enables reuploading     │
└──────────────┬───────────────────────────────────────┘
               │
        [next queued download starts]
               │
└──────────────────────────────────────────────────────┘
```

### GUI ↔ Core Communication (Browse Example)

```
┌──────────────────────────────────────────────────────┐
│ User clicks peer in NetworkWidget                    │
└──────────────┬───────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────────────────┐
│ GUI::RemoteConnection::browse(peerId, path)          │
│ ├─ Serialize CORE_GET_ENTRIES request (protobuf)     │
│ ├─ Send: [MessageHeader|payload] → TCP :59485        │
│ └─ Wait for result                                   │
└──────────────┬───────────────────────────────────────┘
               │
           [Core side]
               │
┌──────────────▼───────────────────────────────────────┐
│ Core::RemoteConnection::onMessageReceived()          │
│ ├─ Deserialize protobuf                              │
│ ├─ Dispatch by type code                             │
│ └─ Call handler (e.g., handleGetEntries)             │
└──────────────┬───────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────────────────┐
│ FileManager::getEntries() [SYNCHRONOUS]              │
│ ├─ Lock fileManagerMutex                             │
│ ├─ Walk Cache tree at path                           │
│ ├─ Build protobuf directory listing                  │
│ ├─ Unlock mutex                                      │
│ └─ Return                                            │
└──────────────┬───────────────────────────────────────┘
               │
┌──────────────▼───────────────────────────────────────┐
│ RemoteConnection::sendResult()                       │
│ ├─ Serialize CORE_GET_ENTRIES_RESULT (protobuf)      │
│ └─ Send back to GUI via TCP                          │
└──────────────┬───────────────────────────────────────┘
               │
           [GUI side]
               │
┌──────────────▼───────────────────────────────────────┐
│ GUI wakes up                                         │
│ ├─ Deserialize result                                │
│ ├─ Update NetworkWidget table                        │
│ └─ User sees file list                               │
└──────────────────────────────────────────────────────┘
```

---

## Design Rationale & Decisions

### Why Protobuf for IPC?

- **Reason:** Type-safe, compact serialization; matches peer-to-peer protocol.
- **Trade-off:** Requires code generation; adds build step. Considered JSON but protobuf is faster.
- **Alternative considered:** Qt serialization (`QDataStream`) — rejected because peer protocol is already protobuf; DRY principle.

### Why Selective Rehosting in Client Mode?

- **Reason:** Client machines often have limited disk/bandwidth. Don't force re-hosting of arbitrary files.
- **Trade-off:** Network less optimal (fewer sources). Mitigation: client users can switch to master mode for shared media libraries.
- **Alternative considered:** Always re-host — simpler but defeats privacy, fills disks.

### Why Raw Pointers in Cache Instead of QSharedPointer?

- **Reason:** Cache is a tree; parent-child relationships are natural with raw pointers and Qt parent-ownership model (auto-delete on parent deletion).
- **Trade-off:** Manual lifetime management; risk of use-after-free if Cache structure changes.
- **Mitigation:** FileUpdater always holds lock; pointers never outlive Cache lock scope.
- **Note:** Code review should verify no pointer escapes without lock protection.

### Why TCP for Peer-to-Peer Instead of UDP?

- **Reason:** Simpler reliability; automatic retransmission. Large file hashes need in-order delivery.
- **Trade-off:** Higher latency; firewall complexity. Mitigation: probing via UDP, once-connected use TCP.
- **Alternative considered:** Custom UDP with acks — complex, Qt lacks robust UDP support.

---

## High-Risk Code Areas (Hotspots)

| Area | Risk | Severity | Mitigation |
|------|------|----------|-----------|
| `FileManager::Cache` access | Use-after-free if pointer escapes lock | **HIGH** | Code review; encapsulate in v2.0 |
| `DownloadManager::FileDownload` state machine | State corruption if transitions not atomic | **MEDIUM** | Add state validation tests |
| `PeerManager::ConnectionPool` reconnect logic | Thundering herd if all peers reconnect | **MEDIUM** | Exponential backoff (exists but verify) |
| `HttpServer::HttpConnection` file seek | Off-by-one in Range header parsing | **MEDIUM** | Add regression tests |
| `FileUpdater` scan performance | Blocks other Core work if folder huge | **LOW** | Profile; consider async hashing in v1.3+ |
| `ChatSystem` (defunct) | Unmaintained code path; bit-rot risk | **MEDIUM** | Mark deprecated; plan removal |

---

## Future Modernization Notes

### Short Term (v1.3)

- [ ] Re-enable DownloadManager tests
- [ ] Add async browsing (don't block Core on large directory walks)
- [ ] Add app-level heartbeat to detect dead peers
- [ ] Document protocol versioning strategy in code
- [ ] Pre-generate protobuf files in build

### Medium Term (v1.4)

- [ ] Prune ChatSystem
- [ ] Refactor Cache to encapsulate pointer ownership
- [ ] Add comprehensive HTTP server tests
- [ ] Implement rate limiting per HTTP client

### Long Term (v2.0)

- [ ] Migrate from qmake to CMake
- [ ] Upgrade to Qt6 (Qt5 EOL December 2025)
- [ ] Replace raw pointers with `std::unique_ptr` throughout
- [ ] Add optional IPv6 support

---

## Related Documents

- **[README.md](README.md)** — User-facing overview, feature list, quick start
- **[BUILD.md](BUILD.md)** — Build instructions, platform-specific setup
- **[PROJECT-CONTEXT.md](PROJECT-CONTEXT.md)** — Build system, version scheme, CI/CD flow
- **[OPERATIONS.md](OPERATIONS.md)** — Windows Service, logging, troubleshooting (see companion doc)
- **[HTTP-SERVER.md](HTTP-SERVER.md)** — HTTP API details
- **[dglan-api/HTTP-STREAMING.md](dglan-api/HTTP-STREAMING.md)** — Python bridge API reference
