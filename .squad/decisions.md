# Squad Decisions

## Active Decisions (Review Hardening Batch — 2026-04-15)

### Phase 0: Triage & Stabilization
- **D1**: Stabilize validation entrypoint before modularization/pruning (Vasquez)
- **D2**: Re-enable DownloadManager tests + establish baseline (Hicks + Vasquez)
- **D3**: Delete 7 obsolete prototypes + fix-protohelper.ps1 (Ripley)
- **D4**: Create TESTING.md and OPERATIONS.md before refactoring (Bishop + Vasquez)

### Phase 1: Safety Net (Weeks 1–3)
- **D5**: Create GUI QTest harness for model classes (Dallas + Ripley)
- **D6**: Extract Protos.pro as static library, eliminate double-compilation (Ripley)
- **D7**: Fix PM::IPeer* raw pointer lifetimes → QWeakPointer (Hicks)
- **D8**: Add RemoteControlManager test suite (Hicks)
- **D9**: Add HttpServer integration tests (Hicks)

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

## Team & Reviews

| Review | Agent | Date | Status |
|--------|-------|------|--------|
| Architecture | Ripley (Lead) | 2026-04-15 | COMPLETE |
| GUI Analysis | Dallas (Frontend) | 2026-04-15 | COMPLETE |
| Backend Validation | Hicks (Backend) | 2026-04-15 | COMPLETE |
| Test Infrastructure | Vasquez (QA) | 2026-04-15 | COMPLETE |
| Documentation & Modernization | Bishop (Docs) | 2026-04-15 | COMPLETE |

---

## Governance

- All meaningful changes require team consensus
- Document architectural decisions here
- Keep history focused on work, decisions focused on direction
- Decisions archived after 30 days (see decisions-archive.md)
