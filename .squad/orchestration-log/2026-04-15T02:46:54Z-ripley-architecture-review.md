# Orchestration Log — Ripley Architecture Review

**Timestamp**: 2026-04-15T02:46:54Z  
**Agent**: Ripley (Lead)  
**Task**: Full codebase architecture review  
**Status**: COMPLETED

## Scope
- Full C++ codebase (488 files, ~48K LOC)
- Python API bridge (dglan-api/)
- Architecture, modularisation, modernisation, and pruning analysis

## Key Findings

### Critical Issues
1. **God Classes** (5,733 LOC in 6 files):
   - NetworkWidget: 1,424 lines
   - UDPListener: 912 lines
   - RemoteConnection: 885 lines
   - ChatWidget: 865 lines
   - Cache: 862 lines
   - SettingsWidget: 785 lines

2. **Raw Pointer Lifetime Hazards**: `PM::IPeer*` raw pointers in ChunkDownloader and FileDownload

3. **Zero GUI Test Coverage**: 121 GUI files, 56 Q_OBJECT classes, 0 tests

4. **Protobuf Build Issue**: `.pb.cc/.pb.h` double-compilation in Common and RemoteCoreController

### Pruning Candidates
- 7 obsolete prototype directories (~1,600 LOC)
- fix-protohelper.ps1 (one-time migration tool)
- Legacy doc/ directory

## Recommendations (Prioritised)

### Phase 1: Safety Net (Weeks 1–3)
- Add GUI QTest harness for models
- Extract Protos.pro library
- Fix peer pointer lifetimes

### Phase 2: Core Modularisation (Weeks 4–7)
- Split god classes: UDPListener, RemoteConnection, Cache

### Phase 3: GUI Modularisation (Weeks 8–11)
- Split NetworkWidget, SettingsWidget, ChatWidget

### Phase 4: Modernisation (Weeks 12–16)
- Smart pointers, CMake migration, Qt6 feasibility

## Deliverables
- **ripley-review.md**: 2.7 KLOC comprehensive analysis
- **Architecture decisions**: 7 team decisions recorded (D1–D7)
- **Execution roadmap**: 5-phase plan with effort estimates

## Team Decisions Made
- D1: Split god classes before adding features
- D2: Establish GUI test harness (Phase 1)
- D3: Fix peer pointer lifetimes before concurrency work
- D4: Stay C++17/Qt, migrate to CMake + Qt6
- D5: Delete 7 obsolete prototypes + fix-protohelper.ps1
- D6: Extract Protos into its own library
- D7: Python API bridge is the quality bar (59 tests, security-first)

## Next Steps
- Dallas: Proceed with GUI dead code removal (Stage 1)
- Hicks: Stabilize backend validation entrypoint
- Vasquez: Build unified test orchestration
- Bishop: Create documentation gap-filling tasks
