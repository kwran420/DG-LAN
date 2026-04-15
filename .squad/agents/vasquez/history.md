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
📌 Implementation Batch 1 complete on 2026-04-15 — Validation entrypoint delivered

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
2. Re-enable DownloadManager tests in legacy runner
3. Add peer discovery smoke tests (multicast → broadcast → fallback)
4. Add network protocol regression targets (browse, hash, download, rehost cycles)
5. Write TESTING.md (C++ and Python test suites, CI gates, manual acceptance)
6. Integrate with GitHub Actions for pre-release validation gates

**Test Matrix (Target Coverage)**:
- **Python bridge**: 59 tests (PASS baseline established)
- **Common module**: 3+ suites
- **FileManager**: Active
- **PeerManager**: Active
- **DownloadManager**: Currently disabled (to be re-enabled)
- **Qt GUI**: 0 tests (future phase)
- **Network discovery**: 0 tests (to be added)
- **Update flow**: 0 tests (to be added)

### 2026-04-15 — Implementation Batch 1: Validation Entrypoint

**Delivered:**
- ✅ `validate.py` (127 lines) created at repo root
- ✅ Probe-based prerequisite detection (pytest, protobuf, bash, qmake, protoc)
- ✅ Classification logic: PASS (all pass), FAIL (test failed), BLOCKED (missing prerequisite)
- ✅ Exit codes: 0 = green, 1 = test failure, 2 = blocked (distinguishes "broken" from "not ready")
- ✅ Layer 1: Python bridge tests (59 cases via pytest)
- ✅ Layer 2: Desktop Qt/C++ tests (legacy runner via 4.run_all_tests.sh)
- ✅ Detailed output with diagnostic info per layer

**Exit Code Strategy:**
- Exit 0: Full validation passed (Python ✅, Desktop ✅)
- Exit 1: At least one layer FAILED (test suite ran but failed)
- Exit 2: At least one layer BLOCKED (prerequisite missing, can't validate)
- Never exits 0 with missing toolchain — prevents accidental green in CI

**Baseline Results** (2026-04-15):
- Python bridge: PASS (59/59 tests)
- Desktop Qt/C++: BLOCKED (qmake/protoc missing in container environment)
- Overall exit code: 2 (BLOCKED)

**Why this first:**
- Establishes baseline for peer lifecycle changes (Ripley/Hicks) and dead code removal (Dallas)
- Enables CI/CD gates that reject "I didn't run the tests" PRs
- Distinguishes deployment readiness (all green) from dev environment gaps (BLOCKED but not broken)

**Next**: Extend with network discovery smoke tests, update flow automation, release checklist enforcement
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
- Legacy Qt test suites can drift out of compile shape as interfaces evolve; before adding regression cases, refresh mocks/stubs to match the current abstract API so the new tests stay reviewable and ready for a real toolchain run.
