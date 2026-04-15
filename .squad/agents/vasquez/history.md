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
📌 **Phase 1 QA Plan COMPLETE** on 2026-04-15 — DownloadManager repair + HttpServer/RemoteControlManager suite priorities finalized
📌 **ID-8 Implemented** on 2026-04-15 — TestsDownloadManager promoted from experimental to mainline validation (4 suites baseline)
📌 **ID-10 Implemented** on 2026-04-15 — HTTP load-balancing QA acceptance matrix recorded; team language corrected (native decentralization vs. peer-redirected HTTP)

## Learnings

The repo already documents Python bridge testing separately from the main desktop application.
- The Python bridge test suite is currently the only validation path I could execute end-to-end here: `cd dglan-api && pip install protobuf && pytest test_streamer.py -v` passed all 59 tests.
- The desktop app has legacy Qt test projects, but the scripted path is brittle: `application/3.compile_all_components.sh` requires `qmake`, and `application/4.run_all_tests.sh` assumes prebuilt binaries and skips some existing test projects.
- A safe first-layer repo validation entrypoint can still be useful in a mixed-stack legacy repo if it classifies each layer as PASS/FAIL/BLOCKED and returns non-zero for blocked desktop coverage instead of faking green status.
- DG-LAN now has a root `validate.py` command plus `TESTING.md`; in this Linux workspace it proves the Python bridge suite and explicitly reports the Qt/protoc desktop gap.
- The current Linux release path is not yet releasable: in this Ubuntu-based workspace `python3 validate.py` failed in the desktop/test layer and `./build-release.sh -SkipPublish` failed in the native Core link step, so “Linux support” must stay evidence-based rather than aspirational.
- Linux support claims need stricter wording than “build script exists”: Raspberry Pi/ARM, Ubuntu/Debian, and RedHat-family support should each require native build + smoke evidence before the tarball is attached to a shared release.

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

### 2026-04-15 — Phase 1 QA Plan Delivery

**Completed**: Comprehensive Phase 1 QA plan document covering DownloadManager repair, suite re-enablement path, and validation entry point clarity.

**Deliverables**:

1. ✅ **DownloadManager Test Repair Checklist**
   - AC-DM-1: 6 acceptance criteria for mock modernization and suite promotion
   - Audit path: Identify stale FileManager API calls in mocks
   - Workstream: 6 tasks (audit → mock update → compile → signal validation → docs → promote)
   - Gate: All AC must pass before suite moved from experimental to wired

2. ✅ **Next Two Highest-Value Suite Promotions**
   - HttpServer (Phase 2, High): 6 AC for range/ETag/CORS/error handling parity (AC-HS-1)
   - RemoteControlManager (Phase 2, High): 6 AC for IPC dispatch/signal/thread safety (AC-RCM-1)
   - Priority ranking: DownloadManager (1) → HttpServer (2) → RemoteControlManager (3) → LogManager (4) → HashCache (5) → NetworkListener (6)

3. ✅ **Validation Entry Point Clarity (TESTING.md wording tightens)**
   - Issue 1 (resolved): `--with-stale-tests` now means "Phase 1 re-enable pending mock modernization" instead of ambiguous "experimental"
   - Issue 2 (resolved): Exit code 2 (BLOCKED) semantics explicit: "not a failure; environment lacks prerequisites; repo is valid"
   - Issue 3 (resolved): Added "Suite Status Definitions" subsection (Wired/Unwired/Discovered/Out-of-scope)
   - Issue 4 (resolved): Added CI/CD gate logic examples (merge gate accepts 0 and 2; release gate requires 0)

4. ✅ **Repo-Level Validation Notes**
   - Baseline metrics established at Phase 0 (Python 59 + C++ 3 wired = 62 total)
   - Phase 1 target: Python 59 + C++ 4 wired (DownloadManager added)
   - Phase 2 target: Python 59 + C++ 6 wired (HttpServer + RemoteControlManager)
   - Desktop toolchain gating principle clarified: distinguish "repo is valid" from "this environment can validate everything"
   - Legacy test maintenance protocol documented (real bug vs. stale mock; experimental classification rationale)

5. ✅ **Team Decision Document**
   - Written to `.squad/decisions/inbox/vasquez-phase1-qa.md` (16.8 KB)
   - Decision: Phase 1 QA Re-enablement Sequence with deliverables, success criteria, timeline, dependencies
   - Timeline: Week 1 DownloadManager → Week 2–3 HttpServer + RemoteControlManager (parallel) → Week 3 integration + CI/CD gates

**Key Findings**:
- 7 test suites discovered in codebase; 3 currently wired (TestsCommon, TestsFileManager, TestsPeerManager)
- DownloadManager is named blocker: mocks reference removed SharedDir/setSharedDirs APIs (no real code bug; mock stale)
- HttpServer and RemoteControlManager test suites do not yet exist; require creation from scratch
- Validation harness (3.compile_all_components.sh + 4.run_all_tests.sh) is mature; --with-stale-tests toggle proven
- TestsLogManager and TestsHashCache are discoverable but lower-priority; NetworkListener requires multicast/broadcast smoke setup

**Impact on Phase 1**:
- DownloadManager repair unblocks Hicks' peer signal infrastructure validation (D7 migration)
- HttpServer + RemoteControlManager suites prevent regressions during Dallas' GUI dead code removal
- Clarity on wired/unwired/discovered terminology prevents contributor confusion
- CI/CD gate examples enable automated enforcement (merge → release progression)

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

## Learnings

- HTTP capability audit finding: the built-in C++ server is currently local-disk HTTP plus master-only redirect; it does not prove decentralized HTTP load balancing because `HttpConnection` redirects only to peers where `isMaster()` and `getHttpPort()>0`, ignoring client rehosters.
- The Python bridge is a single-Core HTTP view, not a decentralized HTTP layer: it streams only files resolvable from the connected Core's shared-entry path index and returns 404 on misses instead of redirecting to other peers.
- `dglan://` is the only path with code evidence for decentralized sourcing today: the URL queues one peer in the GUI, then `DownloadManager`/`UDPListener` can add more peers to chunk downloaders from `CORE_CHUNKS_OWNED` replies, enabling multi-source transfer after handoff.
- QA priority for Hicks' HTTP work should be owner-selection truthfulness before performance language: prove which peers are eligible (master-only vs any rehoster), prevent redirect loops, and validate failover with at least a 3-node master+client+client matrix.
- Current validation gap remains real for this slice: `python3 validate.py` passed the 59 Python bridge tests but desktop validation failed in `TestsCommon::messageHeader()`, so no automated repo gate currently protects built-in HTTP or `dglan://` behavior.

**Pattern 1: Suite Status Terminology**
- Mixed-legacy repos need clear terminology: Wired (in validation profile) vs. Unwired (opt-in) vs. Discovered (found but unassigned) vs. Out-of-scope (intentionally excluded).
- Ambiguous terms like "experimental" or "stale" confuse contributors; explicit phase assignment (Phase 1, Phase 2) is clearer.
- Each suite state should have documented reason and next action; prevents orphaned test projects.

**Pattern 2: Mock Modernization Discipline**
- When real code refactors (e.g., FileManager API changes), legacy test mocks often lag; this creates a choice: (a) fix the mock + re-enable suite (safety-first), or (b) keep suite disabled until mock modernization phase.
- Before declaring a suite "experimental," confirm the breakage is mock stale (not real code bug) and document the required mock updates in an AC (acceptance criterion).
- DownloadManager example: mocks reference removed SharedDir/setSharedDirs; this is mock stale, not DownloadManager real bug; mock modernization is Phase 1 task.

**Pattern 3: Exit Code Semantics in CI/CD**
- Exit code 2 (BLOCKED) is not a failure; CI/CD gates must not treat it as test failure.
- Merge gate logic: accept exit 0 and exit 2; reject only exit 1 (failures).
- Release gate logic: require exit 0; allow native builds on target platform only; exit 2 blocks release (missing toolchain).
- Document gate logic in TESTING.md with concrete bash examples; developers copy-paste.

**Pattern 4: Suite Priority Ranking**
- Rank test suites by risk (what's the worst failure mode?) + coverage gap (Python baseline parity).
- HttpServer (HTTP range parsing) is higher priority than RemoteControlManager (IPC dispatch) because range parsing is security-critical and fewer protocol variants.
- RemoteControlManager (message routing) is higher priority than LogManager (event recording) because routing bugs cause crashes; log bugs cause data loss only.
- Build priority ranking into backlog; don't let lower-risk suites block higher-risk.

**Pattern 5: Baseline Metrics Drive Roadmap**
- Establish Phase 0 baseline: 3 wired C++ suites + 59 Python tests = 62 total regression checks.
- Define Phase 1 target (4 wired C++), Phase 2 target (6 wired C++), etc.
- Track test count growth per phase; use metrics to validate prioritization (should see high-risk suites first).
- Python bridge baseline (59 tests) provides parity target; C++ should reach 80%+ of Python coverage before feature development.

### 2026-04-15 — HTTP Load-Balancing QA Acceptance Criteria

**Outcome**: ✅ COMPLETE — QA acceptance matrix recorded for HTTP and `dglan://` semantics.

**Investigation**: Audited current DG-LAN HTTP behavior across three paths:
1. **Built-in HTTP (C++)**: Local file streaming + master-only fallback redirect (not full decentralization)
2. **Python bridge HTTP**: Single-Core facade with no peer redirect logic (returns 404 on miss)
3. **Native `dglan://`**: Only code-backed multi-source decentralized path (via DownloadManager chunk scheduling)

**Decision (ID-10)**: Establish explicit QA acceptance criteria for HTTP and `dglan://` load-balancing to prevent semantic drift.

**Documentation issues identified**:
- `HTTP-SERVER.md` title "Load Balancing (Already Done)" overstates evidence
- "Distribute load across multiple peers" claim not supported by master-only redirect
- Python bridge docs recommend HEAD support; code rejects non-GET (except OPTIONS)
- README language acceptable narrowly but shouldn't be cited as proof

**Acceptance Matrix**:

**Built-in HTTP (6 criteria)**:
1. Local-hit: GET `/files/{hash}/{path}` returns 200/206 with correct body, no redirect when file exists locally
2. Remote-owner selection: Redirect/forward succeeds when file absent locally but present on eligible peer; clarify: masters-only vs. all HTTP-capable
3. Client rehost coverage: Plain HTTP link works with copies rehosted by client peers
4. Redirect loop prevention: No bouncing 302s; dead peer target fails over or returns deterministic error
5. Owner list truthfulness: `/api/v1/files` `peer_urls` matches actual eligible owners (no narrower subset than docs claim)
6. `dglan://` multi-source retention: Native downloader chunk ownership updates continue adding peers; transfer survives seed peer disappearance

**Python Bridge HTTP (4 criteria)**:
1. GET `/api/v1/files/{hash}/{path}` local success, range success, 416, 304
2. 404 behavior remains explicit (no peer redirect unless explicitly added)
3. Route-level tests for GET/OPTIONS/405 (current 59 tests exercise streamer helpers)
4. Clarify HEAD support in docs or implement if promised

**Team Language** (immediate adoption):
- ✅ Say: "decentralized native downloads" + "peer-redirected HTTP"
- ❌ Avoid: "full decentralized HTTP load balancing" (misleading until acceptance matrix passes)

**Validation Status Observed**:
- `python3 validate.py`: Python bridge PASS (59/59), Desktop Qt FAIL in baseline `TestsCommon::messageHeader()`
- Consequence: No passing automated desktop gate for HTTP/`dglan://` changes yet
- Recommendation: Hicks' work should land with new targeted tests or manual acceptance checklist

**Next Steps** (Phase 1):
1. Create HttpServer integration test suite (5+ tests covering Range, streaming, error cases)
2. Update documentation language to reflect honest semantics
3. Consider native `dglan://` download smoke test
4. Use acceptance matrix in PR reviews for HTTP/download path changes

**Cross-team impact**:
- Hicks implementation (ID-9): HTTP redesigned per this matrix; acceptance pending new tests
- Dallas GUI: No changes to GUI HTTP paths; surface remains stable for dead code removal
- Bishop documentation: Added HTTP-SERVER.md corrections per QA findings
- Team communication: Language standardization prevents future semantic overstatement
