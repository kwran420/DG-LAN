# Project Context

- **Project:** DG-LAN
- **Requested by:** kwran420
- **Tech stack:** C++17 / Qt5 application plus Python API bridge
- **Summary:** DG-LAN needs reliable regression coverage around distributed discovery, transfers, GUI/Core integration, and the Python bridge.

## Core Context

Vasquez owns test strategy, evaluation design, and change-safety recommendations.

## Recent Updates

📌 Team hired on 2026-04-15
📌 Test infrastructure review complete on 2026-04-15 — Validation sprint begins

## Learnings

The repo already documents Python bridge testing separately from the main desktop application.
- The Python bridge test suite is currently the only validation path I could execute end-to-end here: `cd dglan-api && pip install protobuf && pytest test_streamer.py -v` passed all 59 tests.
- The desktop app has legacy Qt test projects, but the scripted path is brittle: `application/3.compile_all_components.sh` requires `qmake`, and `application/4.run_all_tests.sh` assumes prebuilt binaries and skips some existing test projects.
- A safe first-layer repo validation entrypoint can still be useful in a mixed-stack legacy repo if it classifies each layer as PASS/FAIL/BLOCKED and returns non-zero for blocked desktop coverage instead of faking green status.
- DG-LAN now has a root `validate.py` command plus `TESTING.md`; in this Linux workspace it proves the Python bridge suite and explicitly reports the Qt/protoc desktop gap.

### 2026-04-15 — Follow-Up Sprint Assignment

**Role**: Validation Layer Stabilization & Regression Target Prioritization  
**Timeline**: Weeks 1–2 (immediate priority)  
**Deliverables**:
1. Unified test runner script (`run-all-tests.sh` enhancement or new `validate-all.sh`)
2. Re-enable DownloadManager tests in automated flow
3. Add peer discovery smoke test (multicast, broadcast, fallback chain)
4. Add download/rehost cycle validation
5. Create TESTING.md contributor guide (how to run each test suite)

**Key Decisions**:
- D1: Stabilize validation entrypoint before modularization/pruning
- D2: Re-enable DownloadManager tests + establish baseline (Hicks + Vasquez)
- D4: Create TESTING.md before refactoring

**Regression Targets (Priority Order)**:
1. **High**: Network discovery (multicast, broadcast, subnet scan, gossip)
2. **High**: Browse/hash exchange, download/rehost peer-to-peer transfers
3. **High**: HTTP streaming, update checker flow, installer execution
4. **Medium**: Settings migration, cache persistence
5. **Medium**: GUI model interactions, widget state

**Sequencing Notes**:
- Weeks 1–2: Unified test runner + TESTING.md (blocks-nothing prerequisite)
- Week 2 concurrent: DownloadManager tests re-enabled
- Week 2–3: Peer discovery smoke tests begin
- Weeks 3–4: Download/rehost validation automated
- After Week 2: Dallas can proceed with dead code removal (regression baseline in place)

**Risk Mitigation**:
- Python bridge (59 tests) provides reference baseline for network validation
- Smoke tests (discovery, download cycle) cover highest-risk transfer paths
- Phased approach (basic → discovery → transfers) allows early fail detection
