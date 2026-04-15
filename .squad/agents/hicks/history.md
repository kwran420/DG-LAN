# Project Context

- **Project:** DG-LAN
- **Requested by:** kwran420
- **Tech stack:** C++17, protobuf, Qt network/runtime, Python API bridge
- **Summary:** Headless core manages discovery, peer communication, indexing, download orchestration, and remote control for the GUI and HTTP bridge.

## Core Context

Hicks owns networking, service decomposition, and backend modernization analysis.

## Recent Updates

📌 Team hired on 2026-04-15
📌 Backend review complete on 2026-04-15 — Follow-up sprint assigned
📌 Implementation Batch 1 complete on 2026-04-15 — Occupancy hardening delivered

## Learnings

Peer discovery uses multicast, directed broadcast, subnet scan, and gossip fallback.
- Core startup is a tightly coupled service graph rooted in `application/Core/Core.cpp`; the `SETTINGS` singleton macro in `application/Common/Settings.h` still bleeds configuration state directly into backend modules.
- Automated backend coverage is uneven: `application/4.run_all_tests.sh` skips DownloadManager, and there are no sibling test projects for `RemoteControlManager`, `HttpServer`, `UploadManager`, or `ChatSystem`.
- The Python bridge is currently the easiest safe-validation surface here: `dglan-api/test_streamer.py` passed all 59 tests in this workspace, while C++ validation is blocked by missing Qt/qmake/protoc tooling.
- Safe backend pruning in this repo needs two proofs before deletion: zero in-repo references to the candidate path, and a current implementation that already supersedes any one-shot migration helper (for example `ProtoHelper::setStr` already uses `mutable_*`, making `fix-protohelper.ps1` dead).
- In the DownloadManager path, the safest incremental ownership hardening is to move long-lived bookkeeping (`OccupiedPeers`, `LinkedPeers`) to peer IDs while keeping raw `IPeer*` only as short-lived execution handles that are cleared on `peerBecomesUnavailable`.
- The next safe step after peer-ID bookkeeping is to canonicalise every remaining ChunkDownloader peer insert/remove by `Common::Hash`; pointer-equality removal misses rediscovered peer instances and leaves stale transfer handles behind.
- Release automation here is Windows-first: `build-release.ps1` assumes PowerShell plus MSYS2 at `C:\msys64`, MinGW Qt DLLs, and Inno Setup, so Linux containers can only validate via prerequisite checks and `python3 validate.py` before declaring the release build blocked.

### 2026-04-15 — Follow-Up Sprint Assignment

**Role**: Safe Pruning Preparation & Backend Validation  
**Timeline**: Weeks 1–3 (concurrent with Vasquez validation baseline)  
**Deliverables**:
1. RemoteControlManager test suite (5+ tests covering message dispatch)
2. HttpServer integration tests (5+ tests covering Range header, streaming, etc)
3. UploadManager test suite (5+ tests covering multipart upload state)
4. Dead code removal readiness: verify `Client/` CLI usage, `fix-protohelper.ps1` supersession

### 2026-04-15 — Implementation Batch 1: Occupancy Refactoring

**Decision basis**: Ripley's `peerBecomesUnavailable` signal is the enabling infrastructure.

**Delivered:**
- ✅ OccupiedPeers: Migrated from raw `IPeer*` to `Common::Hash` peer ID keys
- ✅ LinkedPeers: Updated to use peer ID bookkeeping (consistent model)
- ✅ DownloadManager: Connected to `peerBecomesUnavailable` signal; proactive cleanup of occupancy sets
- ✅ Defense-in-depth: Transient ChunkDownloader handles preserved; lazy `isAvailable()` checks remain as fallback

**Code areas touched:**
- OccupiedPeers.{h,cpp}: Hash-keyed storage; silent removePeer() for signal cleanup
- LinkedPeers.{h,cpp}: Peer ID bookkeeping
- DownloadManager.{h,cpp}: Signal connection + cleanup logic

**Why this approach:**
- Removes stale-pointer exposure from persistent scheduler state
- Does not change protocol or observable behavior
- Transient ChunkDownloader handles remain defended by lazy checks
- Sets stage for full IPeer* → QSharedPointer migration (ChunkDownloader next)

**Next**: ChunkDownloader transient handle wrapping deferred to future slice (blocked pending Ripley signal ✅ now cleared)
1. Create RemoteControlManager test suite (smoke tests: protocol message routing, auth handling, state)
2. Create HttpServer integration tests (file serving, authentication, range requests)
3. Verify Python bridge tests mirror C++ HTTP server critical paths
4. Coordinate with Dallas: confirm no dead code removal breaks backend
5. Plan RemoteConnection extraction as first modularization target

**Key Decisions**:
- D11: Isolate control-plane boundary via RemoteConnection extraction (phase 2)
- D12: Keep built-in HTTP server canonical; Python bridge as thin adapter
- D17: Harden before feature work — re-enable DownloadManager tests first

**Sequencing Notes**:
- Vasquez validation baseline (week 1–2) enables Hicks backend test development
- Hicks backend tests complete (week 3) before Dallas dead code removal
- RemoteConnection extraction planned for Phase 2 (weeks 4–7) after safety net in place

**Risk Mitigation**:
- Python bridge tests (59) provide reference implementation for C++ server behavior
- Backend tests act as early warning for any dead code removal side effects
- Incremental extraction (RemoteConnection first) reduces refactoring risk

### 2026-04-15 — Phase 0 Closeout Complete

**Status**: ✅ PHASE 0 BACKEND MODERNIZATION COMPLETE

**Orchestration log**: 2026-04-15T04:15:17Z-hicks-backend-modernization.md

**Phase 0 outcomes:**
- ✅ Occupancy refactoring (Hash-keyed OccupiedPeers/LinkedPeers) landed
- ✅ Peer lifecycle signal foundation validated (peerBecomesUnavailable working)
- ✅ Test infrastructure planning complete (RemoteControlManager, HttpServer phases 1–2)
- ✅ Python baseline (59 tests) validates backend safety infrastructure
- 🎯 READY FOR LEAD REVIEW: Peer signal contract + IPeerManager changes

**Phase 1 readiness**: 🎯 TEST INFRASTRUCTURE NEXT
- RemoteControlManager suite (smoke tests, protocol routing, auth handling)
- HttpServer integration tests (file serving, range requests, slow-client detection)
- Validation baseline gates Phase 1 start

**Cross-agent notes**:
- Ripley lead review pending on peerBecomesUnavailable signal contract
- Bishop documentation now complete: CODE-STYLE.md guides safe test harness design
- Vasquez validation baseline (59/59 Python PASS) unblocks C++ test execution
- Dallas GUI analysis done: backend owns test infrastructure, GUI owns dead code removal
- Linux native release proof now exists in this workspace: Qt5 + protobuf packages on Ubuntu 24.04 can build both `DG-LAN.Core` and `DG-LAN.GUI`, but `python3 validate.py` still fails because the legacy Qt test harness contains stale suites (the current failure is `TestsDownloadManager` against removed `SharedDir`/`setSharedDirs` APIs).
- The least-risk Linux release path here is a native per-arch tarball, not cross-distro binary promises: `build-release.sh` should build on the target Linux distro/arch, use top-level `make -j1` to avoid recursive qmake races, and attach the tarball to the same version tag as the Windows installer only after native smoke passes.
- Linux local build/package helpers should not dirty `application/Common/Version.h`: release-only metadata changes belong in `build-release.sh`, while validation/local compile scripts should stay path-safe and restore or skip version rewrites.
- Linux tarball installs must rewrite prefix-sensitive assets at install time (`dglan-core.service`, `dglan.desktop`) and ship provenance (`RELEASE-METADATA.txt`) instead of assuming `/usr/local` or claiming generic cross-distro compatibility.
