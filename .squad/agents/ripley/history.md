# Project Context

- **Project:** DG-LAN
- **Requested by:** kwran420
- **Tech stack:** C++17, Qt5 Widgets, protobuf, qmake / MSYS2 MinGW64, Python API bridge
- **Summary:** DG-LAN distributes a master-curated file list across a LAN with automatic peer discovery, multi-source downloads, and client rehosting.

## Core Context

Ripley owns system review, modularisation strategy, and reviewer gates.

## Recent Updates

📌 Team hired on 2026-04-15
📌 Full review batch complete on 2026-04-15 — Follow-up phase beginning
📌 Implementation Batch 1 complete on 2026-04-15 — Peer lifecycle infrastructure delivered

## Learnings

Initial squad context seeded from README.md and PROJECT-CONTEXT.md.

### 2026-04-15 — Full Architecture Review

**Scope:** Complete codebase audit — 488 C++ files (~48K LOC), Python API bridge, prototypes, build system.

**Key findings:**
- 6 god classes totalling 5,733 LOC are the primary coupling risk (NetworkWidget 1424, UDPListener 912, RemoteConnection 885, ChatWidget 865, Cache 862, SettingsWidget 785)
- Raw `PM::IPeer*` pointers in DownloadManager are a latent crash — peer can be deleted while ChunkDownloader holds reference
- GUI has zero test coverage (121 files, 56 Q_OBJECT classes, 0 tests)
- Core has existing QTest suites for FileManager, PeerManager, DownloadManager, HashCache, Common, LogManager
- Python API bridge (dglan-api/) is the quality exemplar: 59 tests, security-first, excellent docs

### 2026-04-15 — Implementation Batch 1: Peer Lifecycle Infrastructure

**Delivered:**
- ✅ `peerBecomesUnavailable` signal added to IPeerManager (symmetric to `peerBecomesAvailable`)
- ✅ IPeer.h contract fixed (removed misleading "peers are never deleted" statement)
- ✅ Peer::becameDead signal emits on state transition; PeerManager forwards to subscribers
- ✅ ARCHITECTURE.md created (536 lines) — comprehensive design, concurrency model, peer lifecycle state machine
- ✅ OccupiedPeers: Silent removePeer() for signal-based cleanup integration

**Implementation commits:**
- 613c26d5: Add peer death notification signal for proactive cleanup
- 759cc7be: Document peer lifecycle signals in ARCHITECTURE.md
- 821679d2: Update squad artifacts (orchestration logs + implementation decisions)

**Reviewer gate**: D7 (IPeer* → QSharedPointer migration) now unblocked; Lead review required for IPeerManager return type changes

**Next**: Dallas GUI modularization slice (Stage 1); Hicks continues with ChunkDownloader migration
- 7 prototype directories are obsolete and safe to prune
- Generated protobuf files compiled twice (Common + RemoteCoreController) — needs Protos library extraction
- Dependency graph is acyclic (good!) but internal classes violate SRP
- `application/Client/` (CLI client) appears unused — needs owner verification before deletion

**Decisions recorded:** 7 team decisions written to `.squad/decisions.md`
- Split god classes before features, establish GUI tests, fix pointer lifetimes, stay C++17/Qt with CMake+Qt6 path, prune obsolete prototypes, extract Protos library, dglan-api is quality bar

**Execution plan:** 5-phase roadmap (Triage → Safety Net → Core Modularisation → GUI Modularisation → Modernisation) spanning ~16 weeks.

### 2026-04-15 — Follow-Up Batch Kickoff

**Team Assignments**:
- **Vasquez (QA)**: Validation layer stabilization (2 weeks) — unified test runner, DownloadManager tests, peer discovery smoke tests, TESTING.md
- **Bishop (Docs)**: Core documentation (2–3 weeks) — TESTING.md, OPERATIONS.md, CONTRIBUTING.md expansion, architecture history
- **Hicks (Backend)**: Safe pruning prep (3 weeks) — RemoteControlManager test suite, HTTP server tests, dead code removal readiness verification
- **Dallas (GUI)**: Dead code removal (after validation baseline, 2 weeks) — Chat/Emoticons/Activity/Hashing removal, Stage 1 unit tests
- **Ripley**: Sequencing & oversight — ensure phases are de-risked before proceeding

**Synthesized Priorities**:
1. Validation layer first (test baseline enables refactoring)
2. Safe pruning next (dead code removal only after tests confirm no breaks)
3. Modularization after safety net (split god classes, 5-phase sequence)
4. Modernization toward CMake + Qt6 (long-term)

**Success Metrics (next 6 weeks)**:
- Week 1–2: Unified test runner, TESTING.md, DownloadManager tests re-enabled
- Week 3–4: OPERATIONS.md, CONTRIBUTING.md, architecture history
- Week 5–6: RemoteControlManager tests, HTTP server tests, dead code removal readiness

**Cross-agent Dependency Map**: 
Vasquez validation baseline → Bishop documentation → Hicks backend tests → Dallas GUI dead code removal

### 2026-04-15 — First Implementation Slice: Peer Death Notification

**Scope:** Added `peerBecomesUnavailable` signal infrastructure across PeerManager → DownloadManager (10 files, 53 lines net).

**What was done:**
1. Fixed dangerous IPeer.h contract comment ("A peer is never deleted" — wrong)
2. Added `Peer::becameDead()` signal, emitted on alive→dead transition
3. Added `IPeerManager::peerBecomesUnavailable` signal (paired with existing `peerBecomesAvailable`)
4. PeerManager forwards `becameDead` from individual peers to the manager-level signal
5. OccupiedPeers gained `removePeer()` for silent dead-peer removal (no queue-scan trigger)
6. DownloadManager connects to `peerBecomesUnavailable` and proactively cleans occupied-peer sets

**Why this first:**
- Highest-risk gap: no event-driven notification when a peer dies
- Enables D7 (Hicks's QSharedPointer migration) — the signal is the foundation
- Advances all 3 goals: modernization (event-driven), modularization (clean interface), risk reduction (proactive cleanup)
- Small and safe: no behavior change for working downloads, only adds notification path

**Reviewer gate:** Before Hicks starts D7 (IPeer* → QSharedPointer migration), this signal infrastructure must be in place. ✅ Done.

**Follow-up work for next agents:**
- **Hicks (D7):** Migrate IPeer* → QSharedPointer<IPeer> in DownloadManager/ChunkDownloader, using peerBecomesUnavailable as lifecycle hook
- **Hicks (D8):** Add RemoteControlManager test suite (independent, can start now)
- **Dallas (D10-D13):** Dead code removal (Chat/Emoticons/Activity/Hashing) — can start after Vasquez validation baseline

### 2026-04-15 — Linux Port & Release Strategy

**Scope:** Establish working Linux build, release tarball, dual-platform release workflow.

**Key findings from actual build attempt on Ubuntu 24.04 (x86_64):**
1. Both Core and GUI compile and link successfully on Linux with Qt 5.15.13
2. GCC 13 has an internal compiler error (ICE) when LTO is enabled with static archives — fixed by making LTO opt-in
3. The upstream D-LAN project already had extensive `Q_OS_LINUX` guards, qtservice_unix.cpp (daemon support), DirWatcherLinux, WaitConditionLinux — the Linux code paths exist and compile
4. Existing Ubuntu packaging in `application/Setups/Ubuntu/` is outdated (references D-LAN names, SysV init, libprotobuf9)

**Delivered:**
- ✅ `build-linux.sh` — verified Linux build + tarball packaging script (1.4 MB tarball on x86_64)
- ✅ `application/Setups/Linux/dglan-core.service` — modern systemd service file with hardening
- ✅ `application/Setups/Linux/dglan.desktop` — XDG desktop entry for GUI
- ✅ `application/Common/common.pri` — LTO disabled by default on Linux (GCC 13 ICE fix)
- ✅ `BUILD.md` — updated with accurate, tested Linux build instructions
- ✅ Decisions D20-D24 recorded in `.squad/decisions/inbox/ripley-linux-strategy.md`

**Architecture decisions:**
- D20: Tarball + system deps (simplest reliable artifact, works across all distros)
- D21: ARM/Pi is native-build-only (no cross-compilation promise)
- D22: LTO disabled by default on Linux
- D23: Dual-release workflow (both scripts share Version.h, same GitHub Release tag)
- D24: Auto-update doesn't work on Linux (accepted limitation)

**Gotchas identified:**
1. LTO ICE on GCC ≥ 13 (FIXED)
2. Multicast routing may need manual setup on some distros
3. protobuf ABI mismatch between distro versions (must build on target)
4. Auto-update only works for Windows .exe assets
5. GUI requires X11/Wayland display server
6. ARM binaries must be compiled on the target device

### 2026-04-15 — D7 Review Gate Decision

**Scope**: Reviewed peer-lifecycle safety infrastructure for D7 (IPeer* → smart pointer) migration readiness.

**Assessment**:
- ✅ `peerBecomesUnavailable` signal infrastructure correctly wired through full stack
- ✅ OccupiedPeers/LinkedPeers now Hash-keyed (no raw pointers in bookkeeping)
- ✅ IPeer.h contract fixed, ARCHITECTURE.md documents lifecycle state machine
- ✅ Defense-in-depth: signal propagates DownloadQueue → FileDownload → ChunkDownloader

**Verdict**: APPROVED WITH CONSTRAINTS

**Highest-Risk Remaining Seam**: `ChunkDownloader::peers` (`QList<PM::IPeer*>`) and `currentDownloadingPeer` — transient handles during active downloads. Risk is MEDIUM because `peerBecomesUnavailable` mitigates but doesn't eliminate race windows.

**Constraints for D7 Execution**:
1. ChunkDownloader is the scope — do NOT change IPeerManager return types in this slice
2. Preserve signal semantics — new code must still react to `peerBecomesUnavailable`
3. No behavioral changes — verify with existing tests
4. Staged migration — ChunkDownloader first, then FileDownload/DirDownload peerSource

**Recommended Next Slice**: D7-A — ChunkDownloader IPeer* → ID-based lookup with lazy getPeer() resolution

**Key Files Reviewed**:
- `application/Core/PeerManager/priv/Peer.cpp:252-259` — becameDead signal emission
- `application/Core/PeerManager/priv/PeerManager.cpp:277-281` — signal forwarding
- `application/Core/DownloadManager/priv/ChunkDownloader.cpp:320-338` — peerBecomesUnavailable handling
- `application/Core/DownloadManager/priv/OccupiedPeers.cpp:82-94` — silent removePeer()

**Decision Written To**: `.squad/decisions/inbox/ripley-d7-review-gate.md`
