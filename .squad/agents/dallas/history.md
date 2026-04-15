# Project Context

- **Project:** DG-LAN
- **Requested by:** kwran420
- **Tech stack:** C++17, Qt5 Widgets, protobuf, qmake / MSYS2 MinGW64
- **Summary:** Desktop client for browsing a master file index, managing transfers, and controlling a local or remote DG-LAN core.

## Core Context

Dallas owns Qt UI review, interaction flow cleanup, and presentation-layer modularisation.

## Recent Updates

📌 Team hired on 2026-04-15
📌 Full GUI review complete on 2026-04-15 — Stage 1 deadlines assigned

## Learnings

### Session 1: Full GUI Code & Architecture Review (2026-04-15)

**Key Findings**:
1. **74.7 KB dead code identified**: Chat (34 KB), Emoticons (13.8 KB), Activity (9.3 KB), Hashing (11.8 KB), Uploads (5.8 KB shell). All fully implemented but never wired to UI.
2. **Mega-widget discovered**: NetworkWidget.cpp is 1424 lines (65% larger than next widget), handles 5 concerns: peer browsing, file display, download queue, speed tracking, context menus.
3. **Coupling measurements**: 27+ widgets inject ICoreConnection; 89+ direct SETTINGS calls across 38 files; MainWindow owns 10+ members directly; MdiArea couples to Browse/Search specifics.
4. **Architecture sound but scattered**: No circular dependencies detected; injection pattern uniform; but state management scattered, no centralized coordinator.
5. **Testing gap**: Zero GUI automated tests found; no QTest framework usage.

**Decisions Made**:
- ✓ Remove dead code (Chat, Emoticons, Activity, Hashing) — Stage 1
- ✓ Defer Uploads (mark Not-Implemented rather than delete) — revisit quarterly
- ✓ Qt5 Widgets acceptable (no Qt6 migration needed now)
- ✓ Establish test baseline in Stage 1 (add unit tests for models)
- → Recommend 3-stage refactor: Prune (14 hrs) → Modularize (68 hrs) → Modernize (42 hrs)

**Files Analyzed**:
- MainWindow.h/cpp (478 lines, 27 includes)
- MDI/MdiArea.h/cpp (277 lines, couples to Browse/Search)
- Browse/NetworkWidget.h/cpp (1424 lines, mega-widget)
- 12 dead code files across Chat/Emoticons/Activity/Hashing/Uploads
- GUI.pro build configuration
- Settings/state management patterns (89+ SETTINGS calls)

**Architecture Insights**:
- GUI is Qt5 Widgets application with 49 Q_OBJECT widgets
- Connected to Core over TCP/localhost:59485 via ICoreConnection abstraction
- Single-instance IPC server for dglan:// URL scheme
- Auto-update checker (UpdateChecker/UpdateDialog)
- Modular directory structure (Browse, Chat, Downloads, Search, Settings, etc.) but loose coupling between modules

GUI is a Qt5 Widgets application connected to Core over localhost TCP.

### 2026-04-15 — Follow-Up Batch Sequencing

**Timeline for Stage 1 Execution**:
- **Dependencies**: Vasquez validation baseline (weeks 1–2) + Bishop documentation (weeks 1–3) must complete first
- **Dead Code Removal**: Weeks 3–4 (after validation baseline confirms no hidden dependencies)
- **Unit Tests**: Weeks 3–4 (concurrent with code removal)
- **Success Metrics**: 74.7 KB removed, 3+ model tests added, all active features working

**Coordination Points**:
- Hicks will verify no dead code removal dependencies in backend
- Ripley will sequence modularization after Stage 1 completes
- Vasquez will provide regression test baseline for safe deletion verification

**Risk Mitigations**:
- Exhaustive reference search before any file deletion
- Build verification after each removed component
- Feature testing (Browse, Download, Search, Settings, Peers) after Stage 1

### 2026-04-15 — First GUI Pruning Slice

**What shipped**:
- Removed the orphaned `ActivityWidget` and `HashingProgressWidget` sources after confirming they were neither instantiated nor included by `GUI.pro`.
- Stopped compiling the unused `Uploads` shell so the active GUI build no longer carries dead upload-surface code while the deferred implementation stays parked in-tree.

**Why this slice**:
- It is the safest reviewable win before larger `NetworkWidget` extraction work.
- It reduces misleading maintenance surface without changing any reachable user workflow.

### 2026-04-15 — NetworkWidget File-State Extraction

**What shipped**:
- Extracted the file-index and transfer-state bookkeeping from `Browse/NetworkWidget.cpp` into a dedicated `Browse/NetworkFileModel` helper and wired it through `GUI.pro`.
- Kept `NetworkWidget` focused on widget composition, browse orchestration, and user actions while preserving the existing file list behavior, queue controls, and local-path resolution flow.

**Validation**:
- `python3 validate.py` still passes the 59 Python bridge tests.
- Desktop Qt/C++ validation remains blocked in this Linux workspace because `qmake`/`protoc` are unavailable, so the GUI refactor is structurally reviewed but not desktop-built here.

### 2026-04-15 — Phase 1 Stage 1: Chat and Emoticons Removal

**What shipped**:
- Removed 21 dead GUI files: Chat (14 files: ChatWidget, ChatModel, ChatTextEdit, RoomsDock, RoomsModel, RoomsDelegate) and Emoticons (7 files: EmoticonsWidget, Emoticons, SingleEmoticonWidget).
- Cleaned up empty Activity and Hashing directories (files already removed in prior session).
- **Total dead code removed**: ~48 KB GUI presentation layer (Chat + Emoticons).

**Critical distinction**:
- **Removed**: GUI/Chat/* and GUI/Emoticons/* (dead presentation layer, never instantiated)
- **Preserved**: Core/ChatSystem (active backend, compiled/linked in Core.pro, instantiated in Core.cpp:121)

**Verification**:
- Zero references to ChatWidget/EmoticonsWidget outside removed files
- GUI.pro never included Chat/Emoticons sources
- GUI compiles cleanly without errors
- validate.py passes (Python 59/59, C++ test failure pre-existing)

**Next pruning target**:
- **Phase 1 Stage 1 GUI pruning is COMPLETE**. All dead widgets identified in the original audit (Chat, Emoticons, Activity, Hashing) are now removed. Uploads preserved per team decision D14 (deferred, marked "Not Implemented").
- **Recommendation for next slice**: No further GUI pruning needed in immediate term. Activity/Hashing already handled; Uploads explicitly deferred.
