# Orchestration Log — Follow-Up Batch Kickoff

**Timestamp**: 2026-04-15T02:46:54Z  
**Batch**: Review Hardening Follow-Up  
**Agents**: Vasquez (validation), Bishop (docs), Hicks (safe pruning)  
**Status**: KICKOFF

## Batch Scope
Executing high-priority recommendations from completed review batch (Ripley, Dallas, Hicks, Vasquez, Bishop) to establish safety nets before modularization/modernization work.

## Synthesis of Review Batch

### Synthesized Priorities (In Order)
1. **Validation Layer First** (Vasquez + Hicks): Stabilize test entrypoint, enable DownloadManager tests, build automated regression baseline
2. **Safe Pruning Next** (Bishop + Dallas + Ripley): Remove dead code only after validation baseline is in place
3. **Modularization After Safety Net** (Ripley + Dallas + Hicks): Split god classes once tests cover critical paths
4. **Modernization Toward CMake + Qt6** (Ripley + Bishop): Long-term platform upgrades after refactoring

### Decision Consolidation
**Immediate Actions**:
- D1: Stabilize validation entrypoint before modularization/pruning
- D2: Re-enable DownloadManager tests + add RemoteControlManager suite
- D3: Create TESTING.md and OPERATIONS.md documentation
- D4: Remove GUI dead code (Chat, Emoticons, Activity, Hashing) after validation baseline
- D5: Extract Protos.pro library (eliminates double-compilation)
- D6: Fix peer pointer lifetimes (PM::IPeer* → QWeakPointer)

**Phase Sequencing**:
1. Phase 0 (Week 0): Triage + enable existing tests + create docs
2. Phase 1 (Weeks 1–3): GUI dead code removal + test baseline
3. Phase 2 (Weeks 4–7): Core modularization (UDPListener, RemoteConnection, Cache)
4. Phase 3 (Weeks 8–11): GUI modularization (NetworkWidget, SettingsWidget, ChatWidget)
5. Phase 4+ (Weeks 12+): Modernization (CMake, Qt6 feasibility, Rust evaluation)

## Current Follow-Up Team Assignments

### Vasquez (QA) — Validation Layer
**Deliverables** (next 2 weeks):
1. Create unified test runner script (`run-all-tests.sh` enhancement)
2. Re-enable DownloadManager tests
3. Add peer discovery smoke test
4. Add download/rehost cycle validation
5. Draft TESTING.md contributor guide

**Success Criteria**:
- All C++ test suites pass
- Python tests pass (59+)
- Network protocol smoke tests automated
- Documentation enables new contributors to run tests

### Bishop (Docs) — Core Documentation
**Deliverables** (next 2–3 weeks):
1. Create TESTING.md (step-by-step test execution, CI/CD hooks)
2. Create OPERATIONS.md (deployment, monitoring, troubleshooting)
3. Expand CONTRIBUTING.md (code style, PR checklist)
4. Add architecture history section to PROJECT-CONTEXT.md
5. Evaluate CMake migration feasibility

**Success Criteria**:
- Contributors can run all tests without help
- Operators have deployment guidance
- Architecture decisions are documented
- Team consensus on code style

### Hicks (Backend) — Safe Pruning Prep
**Deliverables** (next 3 weeks):
1. Create RemoteControlManager test suite (phase 1: smoke tests only)
2. Design HTTP server integration tests
3. Coordinate dead code removal with Dallas
4. Plan RemoteConnection extraction (Phase 2)
5. Verify Python bridge tests mirror C++ critical paths

**Success Criteria**:
- RemoteControlManager tests baseline coverage
- HTTP server behavior documented in tests
- Dead code removal readiness (no unknown dependencies)
- Python bridge test parity with C++ server

## Risk Mitigations

| Risk | Mitigation |
|------|-----------|
| Validation baseline takes longer than planned | Pre-identify test framework gaps; start with smoke tests only |
| Documentation writing blocks other work | Assign doc writer in parallel while devs work on tests |
| Dead code removal breaks unknown dependencies | Exhaustive search for references before deletion |
| Python bridge tests diverge from C++ | Create shared test specification for HTTP semantics |

## Success Metrics

### Week 1–2 (Validation Baseline)
- ✓ All existing C++ tests passing
- ✓ Python tests passing (59+)
- ✓ Test runner script unified
- ✓ TESTING.md drafted

### Week 3–4 (Documentation)
- ✓ TESTING.md, OPERATIONS.md, CONTRIBUTING.md complete
- ✓ Architecture history documented
- ✓ Dead code removal readiness confirmed

### Week 5–6 (Safe Pruning)
- ✓ RemoteControlManager tests baseline
- ✓ GUI dead code removal starts
- ✓ All active features verified

## Next Handoff Points
1. **After Vasquez validation baseline**: Dallas begins Stage 1 (dead code removal)
2. **After Bishop documentation**: New contributors can run full test suite
3. **After Hicks test suite completion**: Modularization can proceed safely

## Cross-Agent Dependencies
- **Vasquez → Bishop**: Test procedures documented in TESTING.md
- **Bishop → All Devs**: Code style, PR checklist standardized
- **Hicks → Dallas**: Dead code removal only after validation confirms no breaks
- **Vasquez → Hicks**: Test baseline enables RemoteControlManager suite design

---

**Batch Kickoff Complete**  
**Next Review**: Weekly sync on test baseline progress  
**Escalation Point**: If validation baseline takes >3 weeks, reassess modularization sequencing
