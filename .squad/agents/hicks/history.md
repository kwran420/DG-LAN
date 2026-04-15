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

## Learnings

Peer discovery uses multicast, directed broadcast, subnet scan, and gossip fallback.
- Core startup is a tightly coupled service graph rooted in `application/Core/Core.cpp`; the `SETTINGS` singleton macro in `application/Common/Settings.h` still bleeds configuration state directly into backend modules.
- Automated backend coverage is uneven: `application/4.run_all_tests.sh` skips DownloadManager, and there are no sibling test projects for `RemoteControlManager`, `HttpServer`, `UploadManager`, or `ChatSystem`.
- The Python bridge is currently the easiest safe-validation surface here: `dglan-api/test_streamer.py` passed all 59 tests in this workspace, while C++ validation is blocked by missing Qt/qmake/protoc tooling.
- Safe backend pruning in this repo needs two proofs before deletion: zero in-repo references to the candidate path, and a current implementation that already supersedes any one-shot migration helper (for example `ProtoHelper::setStr` already uses `mutable_*`, making `fix-protohelper.ps1` dead).

### 2026-04-15 — Follow-Up Sprint Assignment

**Role**: Safe Pruning Preparation & Backend Validation  
**Timeline**: Weeks 1–3 (concurrent with Vasquez validation baseline)  
**Deliverables**:
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
