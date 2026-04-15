# Squad Decisions

## Active Decisions (Review Hardening Batch — 2026-04-15)

### Phase 0: Triage & Stabilization
- **D1**: Stabilize validation entrypoint before modularization/pruning (Vasquez)
  - ✅ **IMPLEMENTED 2026-04-15**: Created `python3 validate.py` entrypoint (127 lines)
  - Status: PASS (59 Python tests), BLOCKED (desktop tooling in container)
  - Exit codes: 0 = green, 1 = failed, 2 = blocked — prevents accidental green results
- **D2**: Re-enable DownloadManager tests + establish baseline (Hicks + Vasquez)
  - Status: Gating script ready in `validate.py`; awaiting desktop toolchain
- **D3**: Delete 7 obsolete prototypes + fix-protohelper.ps1 (Ripley)
  - Deferred to Phase 1 via Dallas GUI slice
- **D4**: Create TESTING.md and OPERATIONS.md before refactoring (Bishop + Vasquez)
  - ✅ **ARCHITECTURE.md CREATED 2026-04-15**: 536 lines, comprehensive design & peer lifecycle documentation
  - OPERATIONS.md deferred to Phase 1 per Bishop's decision matrix

### Phase 1: Safety Net (Weeks 1–3)
- **D5**: Create GUI QTest harness for model classes (Dallas + Ripley)
  - Deferred to Dallas GUI slice (Stage 1)
- **D6**: Extract Protos.pro as static library, eliminate double-compilation (Ripley)
  - Deferred to Phase 1 proper
- **D7**: Fix PM::IPeer* raw pointer lifetimes → QWeakPointer (Hicks)
  - ✅ **ENABLING SIGNAL INFRASTRUCTURE READY 2026-04-15**: `peerBecomesUnavailable` signal added
  - **Foundation decision**: `peerBecomesUnavailable` is canonical cleanup seam for peer death
  - **Occupancy refactor complete**: OccupiedPeers/LinkedPeers now Hash-keyed (no raw pointers in bookkeeping)
  - **Next gate**: D7 migration proper (ChunkDownloader) may now proceed; awaits lead review
- **D8**: Add RemoteControlManager test suite (Hicks)
  - Deferred to Phase 1 proper
- **D9**: Add HttpServer integration tests (Hicks)
  - Deferred to Phase 1 proper

### Phase 2: GUI Dead Code Removal (Weeks 1–2, after validation baseline)
- **D10**: Remove Chat feature (34 KB, never instantiated) (Dallas)
- **D11**: Remove Emoticons feature (13.8 KB, depends only on Chat) (Dallas)
- **D12**: Remove Activity widget (9.3 KB, orphaned) (Dallas)
- **D13**: Remove Hashing widget (11.8 KB, orphaned) (Dallas)
- **D14**: Mark Uploads as "Not Implemented" (defer deletion, may be planned) (Dallas)

### Phase 2–3: Modularization (Weeks 4+)
- **D15**: Split god classes before feature development (Ripley priority: UDPListener, RemoteConnection, Cache) (All)
- **D16**: Isolate control-plane boundary via RemoteConnection extraction (Hicks)
- **D17**: Keep built-in HTTP server as canonical, Python bridge as thin adapter (Hicks)

### Platform & Modernization (Long-term)
- **D18**: Stay C++17/Qt5 as foundation, migrate to CMake + Qt6 as modernization path (Ripley + Bishop)
- **D19**: Python API bridge is quality baseline (59 tests, security-first) — rest of project should match (All)

---

## Implementation Team & Status (2026-04-15 Batch)

| Agent | Task | Date | Status | Commits |
|-------|------|------|--------|---------|
| Ripley | Peer death signal + ARCHITECTURE.md | 2026-04-15 | ✅ COMPLETE | 613c26d5, 759cc7be |
| Hicks | Occupancy refactoring (Hash-keyed) | 2026-04-15 | ✅ COMPLETE | embedded in peer signal |
| Vasquez | validate.py entrypoint | 2026-04-15 | ✅ COMPLETE | embedded in batch |
| Bishop | Architecture documentation | 2026-04-15 | ✅ COMPLETE (Phase 0) | 759cc7be (ARCHITECTURE.md) |
| Dallas | GUI Analysis (pending execution) | 2026-04-15 | REVIEW PHASE | — |

## Review Gate Status (Original Reviews)

| Review | Agent | Date | Status |
|--------|-------|------|--------|
| Architecture | Ripley (Lead) | 2026-04-15 | COMPLETE |
| GUI Analysis | Dallas (Frontend) | 2026-04-15 | COMPLETE (review) |
| Backend Validation | Hicks (Backend) | 2026-04-15 | COMPLETE |
| Test Infrastructure | Vasquez (QA) | 2026-04-15 | COMPLETE |
| Documentation & Modernization | Bishop (Docs) | 2026-04-15 | COMPLETE |

---

## Implementation Decisions (Batch 1 — 2026-04-15)

### ID-1: Peer Death Notification Infrastructure (Ripley)

**Decision**: Add `peerBecomesUnavailable` signal to IPeerManager as symmetric pair to `peerBecomesAvailable`.

**Rationale**:
- IPeer.h contract incorrectly stated "peers are never deleted"; now corrected
- Peers become dead (isAlive() == false) regularly; DownloadManager needs to react proactively
- Enables D7 (IPeer* → QSharedPointer migration) by providing canonical cleanup seam

**Deliverables**:
- IPeer.h: Fixed contract documentation
- Peer: Added becameDead() signal, emits in consideredDead()
- IPeerManager: Added peerBecomesUnavailable signal
- PeerManager: Forwards signal from Peer instances
- ARCHITECTURE.md: 536-line comprehensive design documentation

**Status**: ✅ IMPLEMENTED 2026-04-15

**Next gate**: D7 migration proper may proceed; lead review required for IPeerManager return type changes

---

### ID-2: Peer Occupancy Refactoring (Hicks)

**Decision**: Migrate long-lived download bookkeeping from raw IPeer* to Common::Hash peer IDs (OccupiedPeers, LinkedPeers).

**Rationale**:
- Removes highest-risk stale-pointer vectors from scheduler-visible state
- Does not change protocol behavior; internal safety improvement only
- Preserves peerBecomesUnavailable/peerBecomesAvailable as cleanup seam
- Transient ChunkDownloader handles defended by existing lazy isAvailable() checks

**Deliverables**:
- OccupiedPeers: Hash-keyed storage; silent removePeer() for cleanup
- LinkedPeers: Peer ID bookkeeping (consistent model)
- DownloadManager: Connect to peerBecomesUnavailable; proactive cleanup

**Status**: ✅ IMPLEMENTED 2026-04-15

**Remaining work**: ChunkDownloader transient handles can wrap in future slice if desired

---

### ID-3: Unified Validation Entrypoint (Vasquez)

**Decision**: Create repo-native `validate.py` (127 lines) that classifies validation as PASS/FAIL/BLOCKED with appropriate exit codes.

**Rationale**:
- Distinguishes "not ready" (BLOCKED, exit 2) from "broken" (FAIL, exit 1)
- Prevents accidental green results when desktop toolchain missing
- Establishes Python bridge baseline (59 tests) as quality bar
- Enables CI/CD pre-release validation gates

**Deliverables**:
- validate.py: Unified entrypoint at repo root
- Layer classification: Python tests (PASS/FAIL/BLOCKED) + Desktop tests (PASS/FAIL/BLOCKED)
- Exit codes: 0 = full green, 1 = test failure, 2 = missing prerequisite

**Status**: ✅ IMPLEMENTED 2026-04-15

**Baseline**: Python bridge 59/59 PASS; Desktop BLOCKED (container environment)

---

### ID-4: Documentation Gap Closure — Phase 0 (Bishop)

**Decision**: Create ARCHITECTURE.md immediately (v1 priority); defer OPERATIONS.md to Phase 1.

**Rationale**:
- ARCHITECTURE.md critical for safe contributor onboarding and code review confidence
- OPERATIONS.md important but can follow; focus on code safety first
- Both docs sourced from actual codebase analysis, not speculation

**Deliverables**:
- ARCHITECTURE.md (536 lines): Subsystem boundaries, concurrency model, key flows, high-risk hotspots, design rationale, modernization paths
- Covers: FileManager, PeerManager, DownloadManager, HttpServer, RemoteControlManager, GUI, peer lifecycle signals
- Documented high-risk areas: Cache pointer ownership (HIGH), ChatSystem (MEDIUM), HTTP Range parsing (MEDIUM)
- Peer lifecycle state machine and signal documentation

**Status**: ✅ COMPLETE 2026-04-15

**Deferred to Phase 1**: OPERATIONS.md (Windows Service, logging, troubleshooting, performance tuning, security, backup/recovery)

---

## Governance

- All meaningful changes require team consensus
- Document architectural decisions here
- Keep history focused on work, decisions focused on direction
- Decisions archived after 30 days (see decisions-archive.md)
